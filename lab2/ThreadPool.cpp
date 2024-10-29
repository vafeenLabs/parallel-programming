#include "ThreadPool.hpp"

void ThreadPool::run()
{
    while (true)
    {
        std::function<void(int)> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this]
                           { return stop || !tasks.empty(); });
            if (stop && tasks.empty())
                return;

            task = std::move(tasks.front());
            tasks.pop();
        }
        task(0); // Выполнение задачи
    }
}