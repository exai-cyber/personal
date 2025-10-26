cmake_minimum_required(VERSION 3.31)
project(password_toolkit)

set(CMAKE_CXX_STANDARD 20)

add_executable(password_toolkit main.cpp
        password_strength.cpp
        password_strength.h
        password_generator.cpp
        password_generator.h)
