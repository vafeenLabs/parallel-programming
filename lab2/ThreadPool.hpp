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
    ThreadPool();

    // Деструктор для завершения работы потоков
    ~ThreadPool();

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    // Метод для добавления задачи в пул и получения результата через future
    std::future<void> enqueue(std::function<void(int)> task);

private:
    // Метод, который выполняется в каждом потоке
    void run();

    std::vector<std::thread> workers;           // Вектор рабочих потоков
    std::queue<std::function<void(int)>> tasks; // Очередь задач
    std::mutex queueMutex;                      // Мьютекс для защиты очереди задач
    std::condition_variable condition;          // Условная переменная для синхронизации
    bool stop;                                  // Флаг остановки пула потоков
};
