#include <iostream>
#include <future>
#include <vector>
#include <algorithm>
#include <ctime>

using namespace std;

const size_t gCount = 100; // количество элементов в массиве
const size_t gThreads = 4; // количество используемых потоков

void init_array(std::vector<int> &arr, int max_element);
double getAVG(vector<double> elements);
void testTimeSync(int size);
void testTimeAsync(int size);
double getTimeSync(int size);
double getTimeAsync(int size);
int partition(std::vector<int> &arr, int low, int high);
void quick_sort(std::vector<int> &arr, int low, int high);
void quick_sort_async(std::vector<int> &arr, int low, int high);

// Инициализация массива случайными числами
void init_array(std::vector<int> &arr, int max_element)
{
    for (size_t i = 0; i < gCount; i++)
    {
        arr[i] = rand() % max_element; // заполняем массив числами от 0 до 99
    }
}

double getAVG(vector<double> elements, bool f)
{
    double sum = 0;
    size_t size = elements.size();
    for (size_t i = 0; i < size; ++i)
    {
        sum += elements[i];
    }
    if (f)
        return sum / size;
    else
        return sum / size * 2;
}

void testTimeSync(int size)
{
    vector<double> times;
    for (size_t i = 0; i < 20; i++)
    {
        double time = getTimeSync(size);
        cout << time << " ";
        times.push_back(time);
    }
    cout << "\nТесты скорости синхронной сортировки: " << getAVG(times, false);
}

void testTimeAsync(int size)
{
    vector<double> times;
    for (size_t i = 0; i < 20; i++)
    {
        double time = getTimeAsync(size);
        cout << time << " ";
        times.push_back(time);
    }
    cout << "\nТесты скорости Aсинхронной сортировки: " << getAVG(times, true);
}

double getTimeAsync(int size)
{
    vector<int> elements(size);
    init_array(elements, size);
    clock_t time_start = clock();
    quick_sort_async(elements, 0, elements.size() - 1);
    double elapsed_time = (double(clock() - time_start)) / CLOCKS_PER_SEC;
    return elapsed_time;
}

double getTimeSync(int size)
{
    vector<int> elements(size);
    init_array(elements, 100);
    clock_t time_start = clock();
    quick_sort(elements, 0, elements.size() - 1);
    clock_t time_end = clock();
    double elapsed_time = double(time_end - time_start) / CLOCKS_PER_SEC;
    return elapsed_time;
}

// Функция для разделения массива (часть алгоритма быстрой сортировки)
int partition(std::vector<int> &arr, int low, int high)
{
    int pivot = arr[high]; // выбираем последний элемент как опорный
    int i = low - 1;       // индекс меньших элементов
    for (int j = low; j < high; j++)
    {
        if (arr[j] < pivot)
        { // если текущий элемент меньше опорного
            i++;
            std::swap(arr[i], arr[j]); // меняем элементы местами
        }
    }
    std::swap(arr[i + 1], arr[high]); // ставим опорный элемент на место
    return i + 1;                     // возвращаем индекс опорного элемента
}

// Рекурсивная функция для быстрой сортировки
void quick_sort(std::vector<int> &arr, int low, int high)
{
    if (low < high)
    {
        int pi = partition(arr, low, high); // разбиваем массив на две части

        // Рекурсивно сортируем левую часть
        quick_sort(arr, low, pi - 1);

        // Рекурсивно сортируем правую часть
        quick_sort(arr, pi + 1, high);
    }
}

// Асинхронная версия быстрой сортировки с использованием потоков
void quick_sort_async(std::vector<int> &arr, int low, int high)
{
    if (low < high)
    {
        int pi = partition(arr, low, high);
        std::future<void> left_sort = std::async(std::launch::async, quick_sort, std::ref(arr), pi + 1, high);
        quick_sort_async(arr, low, pi - 1);
        left_sort.get();
    }
}

int main()
{
    int size1 = 100;
    int size2 = 10000;

    cout << "\n\nTest time sync for " << size1 << "\n";
    testTimeSync(size1);

    cout << "\n\nTest time Async for " << size1 << "\n";
    testTimeAsync(size1);

    cout << "\n\nTest time sync for " << size2 << "\n";
    testTimeSync(size2);

    cout << "\n\nTest time Async for " << size2 << "\n";
    testTimeAsync(size2);
}
