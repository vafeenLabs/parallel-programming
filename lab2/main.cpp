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
        stop = true; // Установка флага остановки
    }
    condition.notify_all(); // Уведомление всех потоков о завершении работы
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
    std::future<returnType> res = task->get_future(); // Получение future для результата задачи
    {
        std::unique_lock<std::mutex> lock(queueMutex); // Блокировка мьютекса
        if (stop) // Проверка, не остановлен ли пул
            throw std::runtime_error("enqueue on stopped ThreadPool"); // Исключение при попытке добавить задачу в остановленный пул

        tasks.emplace([task]() // Добавление задачи в очередь с захватом shared_ptr на задачу
                      { (*task)(); });
    }
    condition.notify_one(); // Уведомление одного потока о наличии новой задачи
    return res; // Возвращение future результата задачи
}

// Функция для расчета числа Фибоначчи рекурсивно
long long fibonacci(int n)
{
    if (n <= 2)
        return n; // Базовые случаи: F(1) = 1, F(2) = 2
    else
        return fibonacci(n - 1) + fibonacci(n - 2); // Рекурсивный вызов для вычисления F(n)
}

// Функция записи текста в файл с указанным именем
void writeToFile(const std::string &filename, const std::string &content)
{
    std::ofstream file(filename); // Открытие файла для записи
    if (file.is_open()) // Проверка успешного открытия файла
    {
        file << content; // Запись содержимого в файл
        file.close(); // Закрытие файла после записи
        std::cout << "Текст записан в файл " << filename << std::endl; // Сообщение об успешной записи
    }
    else
    {
        std::cerr << "Не удалось открыть файл для записи!" << std::endl; // Сообщение об ошибке открытия файла
    }
}

int main()
{
    size_t numThreads = std::thread::hardware_concurrency(); // Получение количества доступных аппаратных потоков
    ThreadPool pool(numThreads); // Создание пула потоков

    int choice;
    while (true)
    {
        std::cout << "Выберите команду (1: Фибоначчи, 2: Запись в файл, 3: Выход): ";
        std::cin >> choice;

        if (choice == 1)
        {
            int n;
            std::cout << "Введите число для расчета Фибоначчи: ";
            std::cin >> n;

            // Используем promise и future для получения результата вычисления числа Фибоначчи в отдельном потоке 
            auto result = pool.enqueue([n]()
                                       {
                long long fibResult = fibonacci(n); // Вычисление числа Фибоначчи 
                std::cout << "Число Фибоначчи для " << n << ": " << fibResult << std::endl;
                return fibResult; }); 
        }
        else if (choice == 2)
        {
            std::string filename, content;
            std::cout << "Введите имя файла: ";
            std::cin >> filename;
            std::cin.ignore(); // Игнорируем оставшийся символ новой строки после ввода имени файла 
            std::cout << "Введите текст для записи в файл: ";
            std::getline(std::cin, content); // Чтение текста с возможными пробелами

            // Записываем текст в файл в отдельном потоке с использованием пула потоков 
            pool.enqueue([filename, content]()
                         { writeToFile(filename, content); });
        }
        else if (choice == 3) 
        {
            break; // Выход из цикла и завершение программы 
        }
        else 
        {
            std::cout << "Неверная команда!" << std::endl; // Сообщение об ошибке ввода команды 
        }
    }

    return 0; // Завершение программы 
}