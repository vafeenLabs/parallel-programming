#include "ThreadPool.hpp"

// Функция для расчета числа Фибоначчи
long long fibonacci(int n)
{
    if (n <= 2)
        return 1;
    else
        return fibonacci(n - 1) + fibonacci(n - 2);
}

// Функция записи текста в файл
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
    ThreadPool pool;

    int choice;
    while (true)
    {
        std::cout << "Выберите команду (1: Фибоначчи, 2: Запись в файл, 3: Выход): ";
        std::cin >> choice;

        if (choice == FIBONACHI_CHOICE)
        {
            int n;
            std::cout << "Введите число для расчета Фибоначчи: ";
            std::cin >> n;

            pool.enqueue([n]()
                         {
                long long fibResult = fibonacci(n);
                std::cout << "Число Фибоначчи для " << n << ": " << fibResult << std::endl; });
        }
        else if (choice == FILE_WRITING_CHOICE)
        {
            std::string filename, content;
            std::cout << "Введите имя файла: ";
            std::cin >> filename;
            std::cin.ignore();
            std::cout << "Введите текст для записи в файл: ";
            std::getline(std::cin, content);

            pool.enqueue([filename, content]()
                         { writeToFile(filename, content); });
        }
        else if (choice == EXIT_CHOICE)
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
