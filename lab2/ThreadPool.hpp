#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <future>
#include <mutex>
#include <condition_variable>

// Класс ThreadPool для управления пулом потоков
class ThreadPool
{
public:
    // Конструктор, инициализирующий пул потоков с заданным количеством потоков
    ThreadPool() : stop(false)
    {
        size_t numThreads = std::thread::hardware_concurrency();
        for (size_t i = 0; i < numThreads; ++i)
        {
            workers.emplace_back([this]
                                 { run(); });
        }
    }

    // Деструктор для завершения работы потоков
    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers)
        {
            worker.join();
        }
    }

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    // Метод для добавления задачи в пул и получения результата через future
    std::future<void> enqueue(std::function<void(int)> task)
    {
        auto taskPtr = std::make_shared<std::packaged_task<void(int)>>(std::move(task));
        std::future<void> res = taskPtr->get_future();

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (stop)
                throw std::runtime_error("enqueue on stopped ThreadPool");
            tasks.emplace([taskPtr](int id)
                          { (*taskPtr)(id); });
        }
        condition.notify_one();
        return res;
    }

private:
    // Метод, который выполняется в каждом потоке
    void run();

    std::vector<std::thread> workers;           // Вектор рабочих потоков
    std::queue<std::function<void(int)>> tasks; // Очередь задач
    std::mutex queueMutex;                      // Мьютекс для защиты очереди задач
    std::condition_variable condition;          // Условная переменная для синхронизации
    bool stop;                                  // Флаг остановки пула потоков
};
