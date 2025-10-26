cmake_minimum_required(VERSION 3.31)
project(password_toolkit)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(password_toolkit
        main.cpp
        password_strength.cpp
        password_generator.cpp
)
