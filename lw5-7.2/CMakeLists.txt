cmake_minimum_required(VERSION 3.20)
project(lw5-7.2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-lzmq)

add_executable(control control.cpp)

add_executable(calculation calculation.cpp)

target_link_libraries(control zmq)
target_link_libraries(calculation zmq)

