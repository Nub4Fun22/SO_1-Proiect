cmake_minimum_required(VERSION 3.28)
project(SO C)

set(CMAKE_C_STANDARD 11)

add_executable(SO main.c
        City_Manager.c
        City_Manager.h)
