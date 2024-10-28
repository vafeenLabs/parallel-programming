#include "ThreadPool.hpp"
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
    if (file.is_open())           // Проверка успешного открытия файла
    {
        file << content;                                               // Запись содержимого в файл
        file.close();                                                  // Закрытие файла после записи
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
    ThreadPool pool(numThreads);                             // Создание пула потоков

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