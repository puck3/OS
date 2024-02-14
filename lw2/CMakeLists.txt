cmake_minimum_required(VERSION 3.10)
project(LW2)

add_executable(lw2_exe main.cpp)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(lw2_exe Threads::Threads)
