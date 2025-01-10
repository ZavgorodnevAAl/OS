cmake_minimum_required(VERSION 3.10)

project(3)

add_executable(3 main.cpp counter.cpp)

target_link_libraries(3 pthread rt)