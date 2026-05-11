# AI Usage Documentation - Phase 1

**Student:** Bordea Sebastian Florin
**AI Tool Used:** Google Gemini

## 1. Generating `parse_condition`

### The Prompt I Gave:
> "I am working on a C project and need to build a filter command. Can you generate a function `int parse_condition(const char *input, char *field, char *op, char *value);` that splits a string formatted as `field:operator:value` (for example `severity:>=:2`) into its three separate string parts?"

### What Was Generated:
The AI generated a function using `sscanf` to slice the string based on the colon `:` character.

>int parse_condition(const char *input, char *field, char *op, char *value) {
if (sscanf(input, "%[^:]:%[^:]:%s", field, op, value) == 3) {
return 1;
}
return 0;
}

## 2. Generating `match_condition`

### The Prompt I Gave:
> "I have a C struct that looks like this:
> typedef struct { int severity; char category[100]; } Report;
> I need a function `int match_condition(Report *r, const char *field, const char *op, const char *value);` that returns 1 if the record satisfies the condition and 0 otherwise. It needs to support ==, !=, <, <=, >, >= for the integer severity, and ==, != for the string category."

### What Was Generated:
The AI generated a series of `if-else` statements comparing the strings, and used `atoi()` to convert the `value` string into an integer before comparing it to the `severity` field.

```c
int match_condition(Report *r, const char *field, const char *op, const char *value) {
    if (strcmp(field, "severity") == 0) {
        int val = atoi(value); 
        if (strcmp(op, "==") == 0) return r->severity == val;
        if (strcmp(op, "!=") == 0) return r->severity != val;
        if (strcmp(op, ">=") == 0) return r->severity >= val;
        if (strcmp(op, "<=") == 0) return r->severity <= val;
        if (strcmp(op, ">") == 0) return r->severity > val;
        if (strcmp(op, "<") == 0) return r->severity < val;
    } else if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->category, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->category, value) != 0;
    }
    return 0;
}
```
## Phase 2: Processes, Signals, and Inter-Process Communication

### 1. Generating Process Management Logic (`fork` and `exec`)

**The Prompt I Gave:**
> "I need to add a `remove_district` command that deletes the district directory using a child process to call `rm -rf`. How do I make the parent wait for the child, but also understand how it could work in parallel?"

**What Was Generated:**
The AI generated the implementation for `remove_district` using `fork()`, `execlp()`, and `waitpid()`. It also provided an explanation of synchronous vs. asynchronous execution.

```c
pid_t pid = fork();
if (pid == 0) {
    execlp("rm", "rm", "-rf", district, NULL);
    exit(1); // Only reached if execlp fails
} else {
    int status;
    waitpid(pid, &status, 0); // Synchronous wait
    // Code to delete symlink here...
}
```
**What I Changed and Why**:
I implemented the synchronous version where the parent uses waitpid() to wait for the child before deleting the active_reports symlink.
This guarantees that the directory is fully removed before the program attempts to clean up the symlink and exit, preventing zombie processes.

**What I Learned:**
>**The exec familiy**:I learned that execlp completely replaces the child process's memory space with the new program (rm). Because it never returns, fork() is absolutely mandatory; otherwise, the City Manager would terminate itself.
>**Parallelism**: I learned that if I placed the symlink deletion before waitpid(), the parent and child would run concurrently (asynchronously).
### 2. Generating Inter-Process Communication (Signals)
**The Prompt I Gave:**
>"Write a new program called monitor_reports that stores its PID in .monitor_pid, ends only on SIGINT, and responds to SIGUSR1. Also modify city_manager to send SIGUSR1 when adding a report."

**What Was Generated:**
> The AI provided the skeleton for monitor_reports.c using sigaction (as required by the assignment, avoiding signal()). It also provided the logic for city_manager to read the .monitor_pid file, use kill() to send the signal, and log the success/failure to logged_district.

**What I Changed and Why:**
During testing, the signals were failing silently. I used AI to generate debug statements to trace the issue. I realized the code was perfectly fine, but the programs were being executed from different Current Working Directories (CWD) inside CLion. I changed my testing methodology to ensure both terminals were in the exact same directory so they could both access the same .monitor_pid file.

**What I Learned:**
>**The Volatile keyword**:I learned that global variables modified inside signal handlers (like keep_running) must be declared as volatile sig_atomic_t. This prevents the compiler from over-optimizing the while loop and ignoring memory updates made by the operating system during a hardware interrupt.

>**Signal Debugging:**:I learned how to isolate IPC bugs by manually sending signals from the terminal (kill -SIGUSR1 <pid>) to test the receiver independently of the sender.

## Phase 3: Pipes and Redirects (Interactive Hub)

### 1. Modifying the Monitor for Pipes (Handling Buffering)

**The Prompt I Gave:**
> "I redirected the monitor's output into a pipe, but now the city_hub is hanging and waiting for messages that never arrive. Why is the pipe frozen?"

**What Was Generated:**
The AI explained the difference between "Line Buffering" (used when printing to a terminal) and "Full Buffering" (used when printing to a pipe or file). It instructed me to add `fflush(stdout);` immediately after every `printf` in `monitor_reports.c`.

**What I Changed and Why:**
I added `fflush(stdout)` to every output statement in my signal handlers and main loop. I also added the required startup check to see if `.monitor_pid` already exists, sending an error message through the pipe and exiting if another monitor was already running.

**What I Learned:**
* **I/O Buffering in C:** I learned that C optimizes writing to pipes by holding onto text until a 4KB buffer is full. `fflush(stdout)` is mandatory to override this and force the program to push small, real-time messages through the pipe instantly.

### 2. Process Redirection (`dup2`) and Multi-Processing

**The Prompt I Gave:**
> "Write the city_hub program that creates a background monitor and reads its pipe. Also, write the calculate_scores function that spawns a separate scorer process for each district and catches its output."

**What Was Generated:**
The AI provided the boilerplate for `city_hub.c` and `scorer.c`. It demonstrated how to use `pipe()`, `fork()`, and `dup2(pipefd[1], STDOUT_FILENO)` to intercept the output of the child processes.

**What I Changed and Why:**
The generated `scorer.c` was initially generic. I provided the AI with my exact `Report` struct from Phase 1 (`City_Manager.h`), and integrated a custom logic block that parses the binary `reports.dat` file, aggregates the severity scores by `inspector_name`, and formats them into a neat table. I also implemented a graceful shutdown sequence in `city_hub` to read `.monitor_pid` and send a `SIGINT` to the background monitor when typing `exit`, preventing orphan processes.

**What I Learned:**
* **Pipe Architecture:** I learned the golden rule of pipes: you must close the unused ends! The parent must close the write end (`pipefd[1]`) to properly read until EOF, and the child must close the read end (`pipefd[0]`).
* **Path Context in `exec`:** When `city_hub` failed to launch the scorer with a "No such file or directory" error, I learned that `execlp("./scorer", ...)` relies entirely on the terminal's Current Working Directory. I had to run the Hub directly from the `cmake-build-debug` folder.

### 3. Fixing CMake Build Targets

**The Prompt I Gave:**
> "I added scorer.c but my build is failing with a 'multiple definition of main' error."

**What Was Generated:**
The AI explained that CLion had automatically grouped `scorer.c` into the existing `monitor_reports` build target.

**What I Changed and Why:**
I updated my `CMakeLists.txt` file to separate each Phase 3 file into its own executable target.
```cmake
add_executable(scorer scorer.c)
add_executable(city_hub city_hub.c)
