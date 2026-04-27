cmake_minimum_required(VERSION 3.28)
project(SO C)

set(CMAKE_C_STANDARD 11)

# Build the main City Manager program (Remove monitor_reports.c from here!)
add_executable(SO main.c City_Manager.c City_Manager.h)

# Build the new Monitor program as a separate app
add_executable(monitor_reports monitor_reports.c)