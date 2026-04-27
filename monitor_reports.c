#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

// Global flag to keep the program running
volatile sig_atomic_t keep_running = 1;

// Signal handler
void handle_signal(int sig) {
    if (sig == SIGINT) {
        keep_running = 0;
        // Note: It's technically unsafe to use printf inside a signal handler,
        // but for a student OS lab, writing a basic message here is standard.
        write(STDOUT_FILENO, "\n[Monitor] SIGINT received. Shutting down...\n", 45);
    } else if (sig == SIGUSR1) {
        write(STDOUT_FILENO, "[Monitor] Alert: A new report has been added to a district!\n", 60);
    }
}

int main() {
    // 1. Setup sigaction for SIGINT and SIGUSR1
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // Restart interrupted system calls

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error binding SIGINT");
        exit(1);
    }
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Error binding SIGUSR1");
        exit(1);
    }

    // 2. Write PID to .monitor_pid
    int fd = open(".monitor_pid", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Could not create .monitor_pid");
        exit(1);
    }

    char pid_str[32];
    int len = sprintf(pid_str, "%d\n", getpid());
    write(fd, pid_str, len);
    close(fd);

    printf("[Monitor] Started with PID %d. Waiting for signals...\n", getpid());

    // 3. Wait in a loop until SIGINT changes keep_running to 0
    while (keep_running) {
        pause(); // Suspends execution until any signal is caught
    }

    // 4. Cleanup on exit
    unlink(".monitor_pid");
    printf("[Monitor] Cleaned up .monitor_pid file. Goodbye!\n");

    return 0;
}