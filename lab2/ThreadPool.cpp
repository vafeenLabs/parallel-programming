#include "ThreadPool.hpp"

void ThreadPool::run()
{
    while (true)
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this]
                           { return stop || !tasks.empty(); });
            if (stop && tasks.empty())
                return;
            task = std::move(tasks.front());
            tasks.pop();
        }

        task();
    }
}

// Метод для добавления задачи в пул и получения результата через future
std::future<void> ThreadPool::enqueue(std::function<void()> task)
{
    auto taskPtr = std::make_shared<std::packaged_task<void(int)>>(std::move(task));
    std::future<void> res = taskPtr->get_future();

    {
        // std::unique_lock<std::mutex> lock(queueMutex);
        std::lock_guard<std::mutex> lock(queueMutex);
        if (stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");
        tasks.emplace([taskPtr](int id)
                      { (*taskPtr)(id); });
    }
    condition.notify_one();
    return res;
}

ThreadPool::ThreadPool() : stop(false)
{
    size_t numThreads = std::thread::hardware_concurrency();
    for (size_t i = 0; i < numThreads; ++i)
    {
        workers.emplace_back([this]
                             { run(); });
    }
}

// Деструктор для завершения работы потоков
ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        // std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread &worker : workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
}