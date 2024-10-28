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
    ThreadPool(size_t numThreads);
    // Деструктор для корректного завершения работы потоков
    ~ThreadPool();

    // Метод для добавления задачи в пул и получения результата через future
    template <class F>
    auto enqueue(F &&f) -> std::future<typename std::result_of<F()>::type>;

private:
    std::vector<std::thread> workers; // Вектор рабочих потоков
    std::queue<std::function<void()>> tasks; // Очередь задач

    std::mutex queueMutex; // Мьютекс для защиты очереди задач
    std::condition_variable condition; // Условная переменная для синхронизации
    bool stop; // Флаг остановки пула потоков
};