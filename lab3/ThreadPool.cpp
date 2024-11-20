#include "ThreadPool.hpp"

void ThreadPool::run()
{
#if defined(_WIN32) || defined(_WIN64)

#else
    while (true)
    {
        std::function<void()> task;

        {
            pthread_mutex_lock(&pthreadMutex);
            while (!stop && tasks.empty())
                pthread_cond_wait(&pthreadCond, &pthreadMutex);
            if (stop && tasks.empty()) 
            {
                pthread_mutex_unlock(&pthreadMutex);
                return;
            }
            if (!tasks.empty())
            {
                task = std::move(tasks.front());
                tasks.pop();
            }
            pthread_mutex_unlock(&pthreadMutex);
        }

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

#else
    {
        pthread_mutex_lock(&pthreadMutex);
        if (stop)
        {

            pthread_mutex_unlock(&pthreadMutex);
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        tasks.emplace([taskPtr]()
                      { (*taskPtr)(); });
        pthread_mutex_unlock(&pthreadMutex);
    }
    pthread_cond_signal(&pthreadCond);
#endif

    return res;
}

ThreadPool::ThreadPool(size_t threads) : stop(false)
{
#if defined(_WIN32) || defined(_WIN64)

#else
    pthread_mutex_init(&pthreadMutex, nullptr);
    pthread_cond_init(&pthreadCond, nullptr);

    for (size_t i = 0; i < threads; ++i)
    {
        pthread_t thread;
        if (pthread_create(&thread, nullptr, [](void *param) -> void *
                           {
                static_cast<ThreadPool *>(param)->run();
                return nullptr; }, this) != 0)
        {
            pthread_mutex_destroy(&pthreadMutex);
            pthread_cond_destroy(&pthreadCond);
            throw std::runtime_error("Failed to create thread");
        }
        workers.emplace_back(thread);
    }
#endif
}

ThreadPool::~ThreadPool()
{
#if defined(_WIN32) || defined(_WIN64)

#else
    {
        pthread_mutex_lock(&pthreadMutex);
        stop = true;
        pthread_mutex_unlock(&pthreadMutex);
    }
    pthread_cond_broadcast(&pthreadCond); // Пробуждаем все потоки (POSIX)
#endif

    for (auto &worker : workers)
    {
#if defined(_WIN32) || defined(_WIN64)

#else
        pthread_join(worker, nullptr);
#endif
    }

#if defined(_WIN32) || defined(_WIN64)

#else
    pthread_mutex_destroy(&pthreadMutex);
    pthread_cond_destroy(&pthreadCond);
#endif
}
