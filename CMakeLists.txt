cmake_minimum_required(VERSION 4.0)
project(ece463_proj3)

set(CMAKE_CXX_STANDARD 20)

add_executable(sim main.cpp
        Simulator.cpp
        Simulator.h)
