#include "ThreadPool.hpp"

// Функция для расчета числа Фибоначчи рекурсивно
long long fibonacci(int n)
{
    if (n <= 2)
        return n;
    else
        return fibonacci(n - 1) + fibonacci(n - 2);
}

// Функция записи текста в файл с указанным именем
void writeToFile(const std::string &filename, const std::string &content)
{
    std::ofstream file(filename);
    if (file.is_open())
    {
        file << content;
        file.close();
        std::cout << "Текст записан в файл " << filename << std::endl;
    }
    else
    {
        std::cerr << "Не удалось открыть файл для записи!" << std::endl;
    }
}

int main()
{
    ThreadPool pool; // Создание пула потоков, число потоков по умолчанию определяется аппаратными возможностями

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

            // Выполняем расчет числа Фибоначчи в отдельном потоке
            pool.enqueue([n](int)
                         {
                long long fibResult = fibonacci(n);
                std::cout << "Число Фибоначчи для " << n << ": " << fibResult << std::endl; });
        }
        else if (choice == 2)
        {
            std::string filename, content;
            std::cout << "Введите имя файла: ";
            std::cin >> filename;
            std::cin.ignore();
            std::cout << "Введите текст для записи в файл: ";
            std::getline(std::cin, content);

            // Записываем текст в файл в отдельном потоке
            pool.enqueue([filename, content](int)
                         { writeToFile(filename, content); });
        }
        else if (choice == 3)
        {
            break;
        }
        else
        {
            std::cout << "Неверная команда!" << std::endl;
        }
    }

    return 0;
}