# City Management System (SO_1-Proiect)

**Author:** Bordea Sebastian Florin
**Institution:** Politehnica University of Timișoara (UPT) - Automation and Computing (CTI)
**Course:** Operating Systems (SO)

## Overview
This project is a multi-phase system-level C application designed to simulate a City Management tracking system. It serves as a practical implementation of core Operating Systems concepts, heavily utilizing Linux system calls for file management, process creation, signal handling, and inter-process communication (IPC).

## Key Features & Project Phases

### Phase 1: Core System & File Management (`SO`)
* **Binary File I/O:** Efficiently reads and writes structured data (reports) to binary files (`reports.dat`) categorized by city districts.
* **Role-Based Access Control:** Simulates basic permission structures based on user roles (e.g., Inspector, Manager) before performing sensitive operations like removing districts.
* **Dynamic Directory Management:** Creates, navigates, and removes district directories using Linux directory traversal and `stat` functions.

### Phase 2: Process Management & Background Daemons (`monitor_reports`)
* **Background Monitoring:** Implements a standalone monitor process that runs in the background, utilizing PID tracking (`.monitor_pid`) to ensure singleton execution.
* **Signal Handling:** Uses `sigaction` to catch custom signals (`SIGUSR1`) sent by the main application when a new report is added, allowing real-time alerts.
* **Zombie Process Prevention:** Employs `waitpid` with the `WNOHANG` flag inside a `SIGCHLD` handler to asynchronously reap child processes (like `rm -rf` operations) without blocking the main program.

### Phase 3: Inter-Process Communication & Interactive Hub (`city_hub`, `scorer`)
* **Interactive CLI:** An interactive shell (`city_hub`) that accepts user commands and spawns background tasks.
* **Pipes & `dup2` Redirection:** The hub launches the monitor and uses anonymous pipes (`pipe()`) combined with file descriptor redirection (`dup2`) to intercept and display the monitor's background output inside the hub's interface.
* **Parallel Processing:** The `calculate_scores` command forks multiple independent `scorer` processes simultaneously (one for each district) to parse binary files and calculate severity scores, aggregating the results back to the hub.
* **Graceful Shutdowns:** Cascading termination sequences ensuring that when the hub exits, it cleanly sends `SIGINT` to background monitors, removing PID files and closing pipes to prevent resource leaks.

## Project Structure & Executables
Compiled via CMake, the project generates four distinct executables:
1. `SO`: The main Phase 1 tool for adding, removing, and modifying reports/districts.
2. `monitor_reports`: The background daemon that listens for file updates and signals.
3. `city_hub`: The interactive console that orchestrates the system, monitors output, and launches scorers.
4. `scorer`: A specialized worker process that reads a specific district's binary file, calculates the workload per inspector, and formats the output.

## Build Instructions
The project is built using CMake. To compile all targets:
```bash
mkdir build
cd build
cmake ..
make