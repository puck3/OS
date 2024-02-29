cmake_minimum_required(VERSION 3.20)
project(lw5-7.2)

include_directories(${CMAKE_SOURCE_DIR})


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_definitions(-lzmq)

add_executable(control control.cpp)

add_executable(calculation calculation.cpp)

target_link_libraries(control Threads::Threads zmq)
target_link_libraries(calculation zmq)


