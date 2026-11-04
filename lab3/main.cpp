// Лабораторная работа 3, вариант 4: многопоточная сортировка TimSort.
//
// Идея TimSort (упрощённая версия):
//   1. Массив делится на маленькие куски длины minrun (32..64 элемента).
//      Каждый кусок сортируется сортировкой вставками (на малых массивах она быстрая).
//   2. Затем отсортированные куски попарно сливаются (merge):
//      сначала куски по minrun, потом по 2*minrun, 4*minrun и т.д.,
//      пока весь массив не станет одним отсортированным куском.
//
// Распараллеливание:
//   - на шаге 1 куски независимы, поэтому раздаём их разным потокам;
//   - на шаге 2 все слияния одного уровня тоже независимы (они не пересекаются),
//     поэтому их тоже раздаём потокам. Следующий уровень начинается только
//     после того, как закончились все потоки предыдущего (pthread_join).
//   Одновременно работает не больше maxThreads потоков (ключ -t).

#include <pthread.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <vector>

// Вычисление minrun так же, как в настоящем TimSort:
// берём 6 старших бит n и добавляем 1, если среди остальных бит есть единица.
int computeMinRun(int n) {
    int r = 0;
    while (n >= 64) {
        r |= n & 1;
        n >>= 1;
    }
    return n + r;
}

// Сортировка вставками отрезка a[left..right] (включительно)
void insertionSort(int* a, int left, int right) {
    for (int i = left + 1; i <= right; i++) {
        int key = a[i];
        int j = i - 1;
        while (j >= left && a[j] > key) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
}

// Слияние двух отсортированных отрезков a[left..mid] и a[mid+1..right]
void merge(int* a, int left, int mid, int right) {
    // копируем левую часть во временный массив, правая остаётся на месте
    std::vector<int> leftPart(a + left, a + mid + 1);
    int leftSize = leftPart.size();

    int i = 0;        // индекс в leftPart
    int j = mid + 1;  // индекс в правой части
    int k = left;     // куда пишем результат

    while (i < leftSize && j <= right) {
        if (leftPart[i] <= a[j]) {
            a[k] = leftPart[i];
            i++;
        } else {
            a[k] = a[j];
            j++;
        }
        k++;
    }
    // остаток левой части дописываем; остаток правой уже стоит на своём месте
    while (i < leftSize) {
        a[k] = leftPart[i];
        i++;
        k++;
    }
}

// Данные, которые получает каждый поток.
// "Блок" - это одна независимая задача: отсортировать один кусок
// или выполнить одно слияние. Поток обрабатывает блоки с firstBlock по lastBlock.
struct ThreadData {
    int* a;
    int n;
    int size;  // длина куска (minrun) или ширина слияния (width)
    int firstBlock;
    int lastBlock;
};

// Поток для шага 1: сортировка вставками своих кусков
void* sortRunsThread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    for (int block = data->firstBlock; block <= data->lastBlock; block++) {
        int left = block * data->size;
        int right = std::min(left + data->size - 1, data->n - 1);
        insertionSort(data->a, left, right);
    }
    return NULL;
}

// Поток для шага 2: слияние пар кусков длины width
void* mergeThread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int width = data->size;
    for (int block = data->firstBlock; block <= data->lastBlock; block++) {
        int left = block * 2 * width;
        int mid = std::min(left + width - 1, data->n - 1);
        int right = std::min(left + 2 * width - 1, data->n - 1);
        if (mid < right) {  // если правой части нет, сливать нечего
            merge(data->a, left, mid, right);
        }
    }
    return NULL;
}

// Делит blocksCount блоков между потоками (не больше maxThreads),
// запускает потоки с функцией threadFunc и ждёт их завершения.
bool runInThreads(void* (*threadFunc)(void*), int* a, int n, int size,
                  int blocksCount, int maxThreads) {
    int threadsCount = std::min(maxThreads, blocksCount);

    std::vector<pthread_t> threads(threadsCount);
    std::vector<ThreadData> data(threadsCount);

    // делим блоки поровну, первые (blocksCount % threadsCount) потоков получают на 1 блок больше
    int blocksPerThread = blocksCount / threadsCount;
    int extra = blocksCount % threadsCount;

    int nextBlock = 0;
    for (int i = 0; i < threadsCount; i++) {
        int count = blocksPerThread;
        if (i < extra) {
            count++;
        }
        data[i].a = a;
        data[i].n = n;
        data[i].size = size;
        data[i].firstBlock = nextBlock;
        data[i].lastBlock = nextBlock + count - 1;
        nextBlock += count;
    }

    int created = 0;
    bool ok = true;
    for (int i = 0; i < threadsCount; i++) {
        int err = pthread_create(&threads[i], NULL, threadFunc, &data[i]);
        if (err != 0) {
            std::cerr << "pthread_create: " << strerror(err) << std::endl;
            ok = false;
            break;
        }
        created++;
    }

    // обязательно дожидаемся всех запущенных потоков
    for (int i = 0; i < created; i++) {
        int err = pthread_join(threads[i], NULL);
        if (err != 0) {
            std::cerr << "pthread_join: " << strerror(err) << std::endl;
            ok = false;
        }
    }
    return ok;
}

bool parallelTimSort(std::vector<int>& array, int maxThreads) {
    int n = array.size();
    if (n < 2) {
        return true;
    }
    int* a = array.data();

    int minRun = computeMinRun(n);

    // шаг 1: сортируем куски длины minRun
    int runsCount = (n + minRun - 1) / minRun;
    if (!runInThreads(sortRunsThread, a, n, minRun, runsCount, maxThreads)) {
        return false;
    }

    // шаг 2: сливаем куски, каждый раз удваивая их длину
    for (int width = minRun; width < n; width *= 2) {
        int mergesCount = (n + 2 * width - 1) / (2 * width);
        if (!runInThreads(mergeThread, a, n, width, mergesCount, maxThreads)) {
            return false;
        }
    }
    return true;
}

void printUsage(const char* name) {
    std::cerr << "usage: " << name << " -t <threads> [-n <size>] [-s <seed>] [-p] [-c]\n"
              << "  -t  max number of threads working at the same time\n"
              << "  -n  array size, up to 500000000 (default 1000000)\n"
              << "  -s  random seed (default 42)\n"
              << "  -p  print array before and after sorting\n"
              << "  -c  check result with std::sort\n";
}

int main(int argc, char* argv[]) {
    int maxThreads = 0;
    int n = 1000000;
    unsigned int seed = 42;
    bool printArray = false;
    bool check = false;

    // разбор ключей запуска
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            maxThreads = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            n = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            seed = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-p") == 0) {
            printArray = true;
        } else if (strcmp(argv[i], "-c") == 0) {
            check = true;
        } else {
            printUsage(argv[0]);
            return 1;
        }
    }

    // ограничение на n, чтобы индексы в merge не вышли за пределы int
    if (maxThreads <= 0 || n <= 0 || n > 500000000) {
        printUsage(argv[0]);
        return 1;
    }

    std::cerr << "pid: " << getpid() << std::endl;

    // заполняем массив случайными числами
    std::vector<int> array(n);
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> distribution(-1000000, 1000000);
    for (int i = 0; i < n; i++) {
        array[i] = distribution(generator);
    }

    if (printArray) {
        for (int x : array) {
            std::cout << x << " ";
        }
        std::cout << std::endl;
    }

    std::vector<int> copy;
    if (check) {
        copy = array;
    }

    auto start = std::chrono::steady_clock::now();
    bool ok = parallelTimSort(array, maxThreads);
    auto finish = std::chrono::steady_clock::now();

    if (!ok) {
        std::cerr << "error: sorting failed" << std::endl;
        return 1;
    }

    double ms = std::chrono::duration<double, std::milli>(finish - start).count();

    if (printArray) {
        for (int x : array) {
            std::cout << x << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "threads: " << maxThreads << ", size: " << n << ", time: " << ms << " ms" << std::endl;

    if (check) {
        std::sort(copy.begin(), copy.end());
        if (copy == array) {
            std::cout << "check: OK" << std::endl;
        } else {
            std::cout << "check: FAILED" << std::endl;
            return 1;
        }
    }
    return 0;
}
