#include <iostream>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "shared_data.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    if (!freopen(argv[1], "w", stdout)) {
        throw runtime_error("File error");
    }

    int shm_fd = shm_open(shm_name, O_RDWR, S_IRUSR | S_IWUSR);
    throw_if(shm_fd, "shared memory open error");

    SharedData* data = (SharedData*) mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (data == MAP_FAILED) {
        throw std::runtime_error("shared memory map error");
    }

    string in;
    while (1) {
        sem_wait(&data->sem1);
        in += data->data;
        sem_post(&data->sem2);
        if (data->end) {
            break;
        }
    }

    int res{0};
    size_t pos{0};
    for (size_t i{0}; i < in.size(); ++i) {
        if (in[i] == '\n' && (i - pos)) {
            res += stoi(in.substr(pos, i - pos));
            pos = i + 1;
            cout << res << endl;
            res = 0;
        } else if (isspace(in[i]) && (i - pos)) {
            res += stoi(in.substr(pos, i - pos));
            pos = i + 1;
        }
    }

    munmap(data, sizeof(SharedData));
    shm_unlink(shm_name);
    fclose(stdout);
    return 0;
}
