#include "ThreadPool.hpp"

void ThreadPool::run()
{
#if defined(_WIN32) || defined(_WIN64)
    while (true)
    {
        std::function<void()> task;

        // Ожидаем, пока не появится новая задача в очереди (семафор)
        DWORD waitResult = WaitForSingleObject(semaphore, INFINITE);
        if (waitResult != WAIT_OBJECT_0)
        {
            // Ошибка ожидания семафора
            throw std::runtime_error("Semaphore wait failed: " + std::to_string(GetLastError()));
        }

        {
            // Блокируем мьютекс, чтобы безопасно работать с очередью задач
            DWORD mutexResult = WaitForSingleObject(winMutex, INFINITE);
            if (mutexResult != WAIT_OBJECT_0)
            {
                // Ошибка ожидания мьютекса
                throw std::runtime_error("Mutex wait failed: " + std::to_string(GetLastError()));
            }

            // Если пул завершен и очередь пуста, выходим
            if (stop || tasks.empty())
            {
                ReleaseMutex(winMutex); // Освобождаем мьютекс
                return;
            }

            // Извлекаем задачу из очереди
            if (!tasks.empty())
            {
                task = std::move(tasks.front());
                tasks.pop();
            }
            ReleaseMutex(winMutex); // Освобождаем мьютекс
        }

        // Выполняем задачу, если она есть
        if (task)
            task();
    }
#else
    while (true)
    {
        std::function<void()> task;

        {
            // Блокируем мьютекс для безопасного доступа к очереди
            pthread_mutex_lock(&pthreadMutex);
            // Ожидаем появления задачи в очереди, если она пуста
            while (!stop && tasks.empty())
                pthread_cond_wait(&pthreadCond, &pthreadMutex);
            // Если пул завершен и очередь пуста, выходим
            if (stop || tasks.empty())
            {
                pthread_mutex_unlock(&pthreadMutex); // Освобождаем мьютекс
                return;
            }

            // Извлекаем задачу из очереди
            if (!tasks.empty())
            {
                task = std::move(tasks.front());
                tasks.pop();
            }
            pthread_mutex_unlock(&pthreadMutex); // Освобождаем мьютекс
        }

        // Выполняем задачу
        if (task)
            task();
    }
#endif
}

std::future<void> ThreadPool::enqueue(std::function<void()> task)
{
    auto taskPtr = std::make_shared<std::packaged_task<void()>>(std::move(task));
    std::future<void> res = taskPtr->get_future();

#if defined(_WIN32) || defined(_WIN64)
    // Блокируем мьютекс перед добавлением задачи в очередь
    DWORD mutexResult = WaitForSingleObject(winMutex, INFINITE);
    if (mutexResult != WAIT_OBJECT_0)
    {
        throw std::runtime_error("Mutex wait failed: " + std::to_string(GetLastError()));
    }

    if (stop)
    {
        ReleaseMutex(winMutex);
        throw std::runtime_error("enqueue on stopped ThreadPool: " + std::to_string(GetLastError()));
    }
    // Добавляем задачу в очередь
    tasks.emplace([taskPtr]()
                  { (*taskPtr)(); });
    ReleaseMutex(winMutex); // Освобождаем мьютекс

    // Уведомляем один поток, что появилась задача
    if (!ReleaseSemaphore(semaphore, 1, nullptr))
    {
        throw std::runtime_error("Failed to release semaphore: " + std::to_string(GetLastError()));
    }

#else
    pthread_mutex_lock(&pthreadMutex);
    if (stop)
    {
        pthread_mutex_unlock(&pthreadMutex);
        throw std::runtime_error("enqueue on stopped ThreadPool: " + std::to_string(errno));
    }

    tasks.emplace([taskPtr]()
                  { (*taskPtr)(); });
    pthread_mutex_unlock(&pthreadMutex); // Освобождаем мьютекс

    pthread_cond_signal(&pthreadCond); // Пробуждаем один поток для выполнения задачи
#endif

    return res;
}

ThreadPool::ThreadPool(size_t threads) : stop(false)
{
#if defined(_WIN32) || defined(_WIN64)
    // Создаем семафор для синхронизации потоков
    semaphore = CreateSemaphore(nullptr, 0, static_cast<LONG>(threads), nullptr);
    if (!semaphore)
    {
        throw std::runtime_error("Failed to create semaphore: " + std::to_string(GetLastError()));
    }

    // Создаем мьютекс для защиты очереди задач
    winMutex = CreateMutex(nullptr, FALSE, nullptr);
    if (!winMutex)
    {
        CloseHandle(semaphore);
        throw std::runtime_error("Failed to create mutex: " + std::to_string(GetLastError()));
    }

    // Создаем рабочие потоки
    for (size_t i = 0; i < threads; ++i)
    {
        HANDLE thread = (HANDLE)_beginthreadex(
            nullptr, 0, [](void *param) -> unsigned
            {
                static_cast<ThreadPool *>(param)->run();
                return 0; },
            this, 0, nullptr);

        if (!thread)
        {
            CloseHandle(semaphore);
            CloseHandle(winMutex);
            throw std::runtime_error("Failed to create thread: " + std::to_string(GetLastError()));
        }
        workers.emplace_back(thread);
    }
#else
    // Инициализация мьютекса и условной переменной для POSIX
    if (pthread_mutex_init(&pthreadMutex, nullptr) != 0)
    {
        throw std::runtime_error("Failed to initialize mutex: " + std::to_string(errno));
    }
    if (pthread_cond_init(&pthreadCond, nullptr) != 0)
    {
        pthread_mutex_destroy(&pthreadMutex);
        throw std::runtime_error("Failed to initialize condition variable: " + std::to_string(errno));
    }

    // Создаем рабочие потоки
    for (size_t i = 0; i < threads; ++i)
    {
        pthread_t thread;
        // Тут вернется 0, если поток успешно создан
        if (pthread_create(&thread, nullptr, [](void *param) -> void *
                           {
                               static_cast<ThreadPool *>(param)->run();
                               return nullptr; }, this) != 0)
        {
            pthread_mutex_destroy(&pthreadMutex);
            pthread_cond_destroy(&pthreadCond);
            throw std::runtime_error("Failed to create thread: " + std::to_string(errno));
        }
        workers.emplace_back(thread);
    }
#endif
}
ThreadPool::~ThreadPool()
{
#if defined(_WIN32) || defined(_WIN64)
    // Блокируем мьютекс перед установкой флага stop
    WaitForSingleObject(winMutex, INFINITE);
    stop = true;
    ReleaseMutex(winMutex);

    // Уведомляем все потоки, чтобы они завершились
    for (size_t i = 0; i < workers.size(); ++i)
    {
        ReleaseSemaphore(semaphore, 1, nullptr);
    }

    // Завершаем все потоки, освобождая ресурсы 
    for (auto &worker : workers)
    {
        CloseHandle(worker);
    }

    // Освобождаем ресурсы
    CloseHandle(semaphore);
    CloseHandle(winMutex);
#else
    // POSIX реализация
    pthread_mutex_lock(&pthreadMutex);
    stop = true;
    pthread_mutex_unlock(&pthreadMutex);

    pthread_cond_broadcast(&pthreadCond); // Пробуждаем потоки

    // Завершаем потоки
    for (auto &worker : workers)
    {
        pthread_join(worker, nullptr);
    }

    // Освобождаем ресурсы
    pthread_mutex_destroy(&pthreadMutex);
    pthread_cond_destroy(&pthreadCond);
#endif
}