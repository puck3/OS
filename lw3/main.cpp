#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/wait.h>
#include "shared_data.hpp"

using namespace std;

int main() {
    freopen("in", "r", stdin);

    int err;
    string file;
    cout << "Enter file name: ";
    cin >> file;
    cout << "Enter commands:" << endl;

    int shm_fd = shm_open(shm_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    throw_if(shm_fd, "shared memory open error");

    err = ftruncate(shm_fd, sizeof(SharedData));
    throw_if(err, "shared memory truncate error");

    SharedData* data = (SharedData*) mmap(NULL, sizeof(SharedData), PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (data == MAP_FAILED) {
        throw std::runtime_error("Ошибка при отображении shared memory");
    }

    err = sem_init(&data->sem1, 1, 0);
    throw_if(err, "semaphore init error");

    err = sem_init(&data->sem2, 1, 0);
    throw_if(err, "semaphore init error");

    pid_t pid = fork();
    throw_if(pid, "fork failed");
    if (pid == 0) {
        err = execl("lw3_child_proc", "lw3_child_proc", file.c_str(), NULL);
        throw_if(err, "Child file error");
    } else {
        string s, out = "";
        while (getline(cin, s)) {
            if (s.size()) s += '\n';
            out += s;
        }

        data->data[15] = '\0';
        for (int i{0}; i < out.size(); ++i) {
            if (!(i % 15) && i) {
                sem_post(&data->sem1);
                sem_wait(&data->sem2);
            }
            data->data[i % 15] = out[i];
        }
        if (!out.size() % 15) {
            data->data[out.size() % 15] = '\0';
        }
        data->end = true;
        sem_post(&data->sem1);

        wait(nullptr);
    }
    sem_destroy(&data->sem1);
    sem_destroy(&data->sem2);

    munmap(data, sizeof(SharedData));
    shm_unlink(shm_name);
}
