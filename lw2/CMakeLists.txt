cmake_minimum_required(VERSION 3.10)
project(LW2)

add_executable(main_exe main.cpp)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(main_exe PRIVATE Threads::Threads)
