#include "functions.hpp"
#include <cmath>

double E(int x) {
    return pow((1 + 1 / static_cast<double>(x)), x);
}

int* Sort(int size, int* array) {
    bool sorted;
    for (int i{0}; i < size; ++i) {
        sorted = true;
        for (int j{0}; j < size - i - 1; ++j) {
            if (array[j] > array[j + 1]) {
                std::swap(array[j], array[j + 1]);
                sorted = false;
            }
        }
        if (sorted) break;
    }
    return array;
}
