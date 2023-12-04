#include <iostream>
#include "functions.hpp"

int main() {
    int n;
    while (std::cin >> n) {
        if (n == 1) {
            int x;
            std::cin >> x;
            double res = E(x);
            std::cout << res << std::endl;
        } else if (n == 2) {
            int size;
            std::cin >> size;
            int* arr = new int[size];
            for (int i{0}; i < size; ++i) {
                std::cin >> arr[i];
            }
            arr = Sort(size, arr);
            for (int i{0}; i < size; ++i) {
                std::cout << arr[i] << " ";
            }
            std::cout << std::endl;
            delete[] arr;
        } else {
            break;
        }

    }
}