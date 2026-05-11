#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

volatile sig_atomic_t keep_running = 1;

void handle_signal(int sig) {
    if (sig == SIGINT) {
        keep_running = 0;
        printf("[EXIT] SIGINT received. Shutting down...\n");
        fflush(stdout); // CRITICAL: Forces the text into the pipe immediately
    } else if (sig == SIGUSR1) {
        printf("[ALERT] A new report has been added to a district!\n");
        fflush(stdout);
    }
}

int main() {
    // --- PHASE 3: Check if already running ---
    int check_fd = open(".monitor_pid", O_RDONLY);
    if (check_fd >= 0) {
        char existing_pid[32];
        memset(existing_pid, 0, sizeof(existing_pid));
        read(check_fd, existing_pid, sizeof(existing_pid) - 1);
        close(check_fd);

        // Write the specific error to the pipe and exit
        printf("[ERR] Monitor is already running with PID: %s\n", existing_pid);
        fflush(stdout);
        return 1;
    }

    // 1. Setup sigaction
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);

    // 2. Write PID
    int fd = open(".monitor_pid", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Could not create .monitor_pid");
        exit(1);
    }
    char pid_str[32];
    int len = sprintf(pid_str, "%d\n", getpid());
    write(fd, pid_str, len);
    close(fd);

    printf("[START] Monitor Started with PID %d. Waiting for signals...\n", getpid());
    fflush(stdout); // CRITICAL

    // 3. Wait loop
    while (keep_running) {
        pause();
    }


    // 4. Cleanup
    unlink(".monitor_pid");
    printf("[EXIT] Cleaned up .monitor_pid file. Goodbye!\n");
    fflush(stdout);

    return 0;
}