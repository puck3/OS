cmake_minimum_required(VERSION 3.10)
project(BattleShip)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(client client.cpp)
add_executable(server server.cpp)

target_link_libraries(server Threads::Threads zmq)


