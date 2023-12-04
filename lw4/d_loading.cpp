#include <iostream>
#include <dlfcn.h>

typedef double (*EFunction)(int);
typedef int* (*SortFunction)(int, int*);

EFunction E = nullptr;
SortFunction Sort = nullptr;
void* libraryHandle = nullptr;

void load_lib(const char* file) {
    libraryHandle = dlopen(file, RTLD_LAZY);
    if (!libraryHandle) {
        throw  std::runtime_error(dlerror());
    }

    E = (EFunction) dlsym(libraryHandle, "E");
    if (!E) {
        throw  std::runtime_error(dlerror());
    }
    Sort = (SortFunction) dlsym(libraryHandle, "Sort");
    if (!Sort) {
        throw  std::runtime_error(dlerror());
    }
}

int main() {
    load_lib("libimpl1.so");

    int current_impl{1};
    int n;

    while (std::cin >> n) {
        if (n == 0) {
            dlclose(libraryHandle);
            switch (current_impl) {
                case 1:
                    load_lib("libimpl2.so");
                    current_impl = 2;
                    break;

                default:
                    load_lib("libimpl1.so");
                    current_impl = 1;
                    break;
            }

        } else if (n == 1) {
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
    dlclose(libraryHandle);

    return 0;
}