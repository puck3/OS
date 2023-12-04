#include "functions.hpp"
#include <cmath>

std::pair<int, int> partition(int* a, int begin, int end, int pivot) {
    int lt_end{begin}, eq_end{begin};
    for (int i{begin}; i < end; ++i) {
        if (a[i] < pivot) {
            std::swap(a[eq_end], a[lt_end]);
            if (eq_end != i) {
                std::swap(a[lt_end], a[i]);
            }
            ++lt_end; ++eq_end;
        } else if (a[i] == pivot) {
            std::swap(a[eq_end], a[i]);
            ++eq_end;
        }
    }
    return std::pair<int, int>(lt_end, eq_end);
}

void qsort(int* a, int begin, int end) {
    if ((end - begin) < 2) return;
    int pivot = a[begin + rand() % (end - begin)];

    std::pair<int, int> i = partition(a, begin, end, pivot);

    qsort(a, begin, i.first);
    qsort(a, i.second, end);
}


double E(int x) {
    long long fact = 1;
    double sum{1};
    for (int i{1}; i < x; ++i) {
        fact *= i;
        sum += 1 / static_cast<double>(fact);
    }
    return sum;
}

int* Sort(int size, int* array) {
    qsort(array, 0, size);
    return array;
}
