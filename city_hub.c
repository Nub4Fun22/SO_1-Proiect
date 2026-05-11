#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_INPUT 256
#define MAX_DISTRICTS 10

// Function to handle the start_monitor command
void start_monitor() {
    pid_t hub_mon_pid = fork();

    if (hub_mon_pid == 0) {
        // --- WE ARE IN HUB_MON ---
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("Pipe failed");
            exit(1);
        }

        pid_t monitor_pid = fork();

        if (monitor_pid == 0) {
            // --- WE ARE IN THE MONITOR ---
            // 1. Close the read end of the pipe (Monitor only writes)
            close(pipefd[0]);

            // 2. Redirect standard output into the pipe
            dup2(pipefd[1], STDOUT_FILENO);

            // 3. Close the original write pipe (dup2 made a copy)
            close(pipefd[1]);

            // 4. Transform into the monitor executable
            execlp("./monitor_reports", "monitor_reports", NULL);
            perror("execlp failed");
            exit(1);
        } else {
            // --- WE ARE BACK IN HUB_MON ---
            // 1. Close the write end of the pipe (Hub_mon only reads)
            close(pipefd[1]);

            char buffer[256];
            int bytes_read;

            // 2. Read from the pipe continuously until the monitor closes it
            while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
                buffer[bytes_read] = '\0';

                // Prefix the output so the user knows it came from the background monitor
                printf("\n[HUB_MON INTERCEPT] %s", buffer);
            }

            // 3. The read() loop only breaks when the monitor completely exits
            close(pipefd[0]);
            printf("\n[HUB_MON ALERT] The background monitor process has officially terminated.\n");

            // Hub_mon's job is done
            exit(0);
        }
    } else {
        // --- WE ARE IN CITY_HUB ---
        // We do NOT waitpid here! The assignment says "creates a background child process".
        printf("Background hub_mon launched! Returning to command prompt...\n");
    }
}

// Function to handle calculate_scores
void calculate_scores(char* args) {
    char *districts[MAX_DISTRICTS];
    int count = 0;

    // Tokenize the list of districts
    char *token = strtok(args, " \n");
    while (token != NULL && count < MAX_DISTRICTS) {
        districts[count++] = token;
        token = strtok(NULL, " \n");
    }

    if (count == 0) {
        printf("Error: Please provide at least one district.\n");
        return;
    }

    int pipes[MAX_DISTRICTS][2];
    pid_t pids[MAX_DISTRICTS];

    printf("\n=== COMBINED WORKLOAD REPORT ===\n");

    // Spawn a scorer for each district
    for (int i = 0; i < count; i++) {
        pipe(pipes[i]);
        pids[i] = fork();

        if (pids[i] == 0) {
            // Child (Scorer)
            close(pipes[i][0]); // Close read end
            dup2(pipes[i][1], STDOUT_FILENO); // Redirect stdout to pipe
            close(pipes[i][1]);

            execlp("./scorer", "scorer", districts[i], NULL);
            perror("execlp scorer failed");
            exit(1);
        } else {
            // Parent (City Hub)
            close(pipes[i][1]); // Close write end immediately
        }
    }

    // Collect outputs from all pipes
    for (int i = 0; i < count; i++) {
        char buffer[1024];
        int bytes_read;

        while ((bytes_read = read(pipes[i][0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            printf("%s", buffer);
        }
        close(pipes[i][0]);

        // Wait for this specific child to finish cleanly
        int status;
        waitpid(pids[i], &status, 0);
    }
    printf("================================\n");
}

int main() {
    // Tell City Hub to ignore SIGCHLD to prevent hub_mon from becoming a zombie
    // since we run it in the background without waitpid().
    signal(SIGCHLD, SIG_IGN);

    char input[MAX_INPUT];

    printf("Welcome to City Hub CLI. Type 'exit' to quit.\n");

    while (1) {
        printf("city_hub> ");
        if (fgets(input, sizeof(input), stdin) == NULL) break;

        // Remove trailing newline
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "exit") == 0) {
            // --- GRACEFUL SHUTDOWN SEQUENCE ---
            printf("Initiating shutdown sequence...\n");

            // 1. Try to open the PID file to see if a monitor is running
            FILE *pid_file = fopen(".monitor_pid", "r");
            if (pid_file) {
                pid_t monitor_pid;

                // 2. Read the PID and send the kill signal
                if (fscanf(pid_file, "%d", &monitor_pid) == 1) {
                    printf("Sending SIGINT to background monitor (PID %d)...\n", monitor_pid);
                    kill(monitor_pid, SIGINT);
                }
                fclose(pid_file);

                // 3. Give the monitor 1 second to delete the file and close the pipe
                sleep(1);
            }

            printf("City Hub shutting down. Goodbye!\n");
            break;
        } else if (strncmp(input, "start_monitor", 13) == 0) {
            start_monitor();
        } else if (strncmp(input, "calculate_scores", 16) == 0) {
            // Pass the rest of the string (the district list) to the function
            char *args = input + 16;
            calculate_scores(args);
        } else if (strlen(input) > 0) {
            printf("Unknown command. Valid commands: start_monitor, calculate_scores <d1> <d2>...\n");
        }
    }

    return 0;
}
