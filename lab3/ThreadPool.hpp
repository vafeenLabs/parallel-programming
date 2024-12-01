#pragma once 
#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <functional>
#include <future>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <pthread.h>
#endif

enum Point
{
    FIBONACHI_CHOICE = 1,
    FILE_WRITING_CHOICE,
    EXIT_CHOICE,
    DESCRIPTION_CHOICE,

};

// Класс ThreadPool для управления пулом потоков
class ThreadPool
{
public:
    // Конструктор для инициализации пула потоков с заданным количеством потоков
    ThreadPool(size_t threads = std::thread::hardware_concurrency());

    // Деструктор завершает работу потоков
    ~ThreadPool();

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    // Метод для добавления задачи в пул и получения результата через future
    std::future<void> enqueue(std::function<void()> task);
private:

    void run();

    std::queue<std::function<void()>> tasks;

    bool stop;

#if defined(_WIN32) || defined(_WIN64)
    // Для Windows: массив дескрипторов потоков
    std::vector<HANDLE> workers;

    // Семафор для управления ожиданием задач
    HANDLE semaphore;

    // Мьютекс для защиты очереди задач
    std::mutex queueMutex;

#else
    // Для POSIX: массив идентификаторов потоков pthread
    std::vector<pthread_t> workers;

    // Мьютекс для синхронизации потоков и очереди
    pthread_mutex_t pthreadMutex;

    // Условная переменная для потоков (POSIX)
    pthread_cond_t pthreadCond;
#endif
};
