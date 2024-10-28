#include "ThreadPool.hpp"

// Конструктор, запускающий заданное количество потоков
ThreadPool::ThreadPool(size_t numThreads) : stop(false)
{
    for (size_t i = 0; i < numThreads; ++i)
    {
        workers.emplace_back([this] // Лямбда-функция для выполнения задач
                             {
            for (;;) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(this->queueMutex); // Блокировка мьютекса
                    this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                    // Ожидание, пока не появится задача или не будет установлено завершение
                    if (this->stop && this->tasks.empty()) return; // Выход из цикла, если остановка пула и нет задач
                    task = std::move(this->tasks.front()); // Получение задачи из очереди
                    this->tasks.pop(); // Удаление задачи из очереди
                }

                task(); // Выполнение задачи
            } });
    }
}

// Деструктор для завершения работы потоков
ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queueMutex); // Блокировка мьютекса
        stop = true;                                   // Установка флага остановки
    }
    condition.notify_all();             // Уведомление всех потоков о завершении работы
    for (std::thread &worker : workers) // Ожидание завершения всех потоков
    {
        worker.join();
    }
}

// Функция добавления задачи в пул потоков
template <class F>
auto ThreadPool::enqueue(F &&f) -> std::future<typename std::result_of<F()>::type>
{
    using returnType = typename std::result_of<F()>::type; // Определение типа возвращаемого значения

    auto task = std::make_shared<std::packaged_task<returnType()>>(std::forward<F>(f)); // Создание задачи
    std::future<returnType> res = task->get_future();                                   // Получение future для результата задачи
    {
        std::unique_lock<std::mutex> lock(queueMutex);                 // Блокировка мьютекса
        if (stop)                                                      // Проверка, не остановлен ли пул
            throw std::runtime_error("enqueue on stopped ThreadPool"); // Исключение при попытке добавить задачу в остановленный пул

        tasks.emplace([task]() // Добавление задачи в очередь с захватом shared_ptr на задачу
                      { (*task)(); });
    }
    condition.notify_one(); // Уведомление одного потока о наличии новой задачи
    return res;             // Возвращение future результата задачи
}