#pragma once
#include <string>
#include <semaphore.h>

const char* shm_name = "/my_shared_memory";

void throw_if(int err, string what) {
    if (err == -1) {
        throw runtime_error(what);
    }
}

struct SharedData {
public:
    bool end = false;
    char data[16];
    sem_t sem1;
    sem_t sem2;
    sem_t sem3;
};
