cmake_minimum_required(VERSION 3.28)
project(SO C)

set(CMAKE_C_STANDARD 11)

# Phase 1 & 2 Executables
add_executable(SO main.c City_Manager.c City_Manager.h)
add_executable(monitor_reports monitor_reports.c)

# Phase 3 Executables
add_executable(scorer scorer.c)
add_executable(city_hub city_hub.c)