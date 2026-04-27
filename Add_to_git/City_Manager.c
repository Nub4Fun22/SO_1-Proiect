#include "City_Manager.h"

// Converts st_mode bits to a 9-character string
void mode_to_string(mode_t mode, char* str) {
    strcpy(str, "---------");
    if (mode & S_IRUSR) str[0] = 'r';
    if (mode & S_IWUSR) str[1] = 'w';
    if (mode & S_IXUSR) str[2] = 'x';
    if (mode & S_IRGRP) str[3] = 'r';
    if (mode & S_IWGRP) str[4] = 'w';
    if (mode & S_IXGRP) str[5] = 'x';
    if (mode & S_IROTH) str[6] = 'r';
    if (mode & S_IWOTH) str[7] = 'w';
    if (mode & S_IXOTH) str[8] = 'x';
}

int check_permissions(const char* filepath, const char* role, int check_write) {
    struct stat info;
    if (stat(filepath, &info) < 0) {
        return 1; // File might not exist yet; handle at open()
    }

    if (strcmp(role, "manager") == 0) {
        if (check_write && !(info.st_mode & S_IWUSR)) return 0;
        if (!check_write && !(info.st_mode & S_IRUSR)) return 0;
    } else if (strcmp(role, "inspector") == 0) {
        if (check_write && !(info.st_mode & S_IWGRP)) return 0;
        if (!check_write && !(info.st_mode & S_IRGRP)) return 0;
    } else {
        return 0;
    }
    return 1;
}

void add_report(const char* district, const char* role, const char* user) {
    struct stat st;

    if (stat(district, &st) < 0) {
        if (mkdir(district, 0750) < 0) {
            perror("mkdir failed");
            exit(1);
        }
    }

    char filepath[256];
    sprintf(filepath, "%s/reports.dat", district);

    if (!check_permissions(filepath, role, 1)) {
        printf("Permission denied: Role '%s' cannot write to %s\n", role, filepath);
        exit(1);
    }

    int fd = open(filepath, O_CREAT | O_WRONLY | O_APPEND, 0664);
    if (fd < 0) {
        perror("open reports.dat failed");
        exit(1);
    }

    chmod(filepath, 0664);

    Report r;
    r.report_id = time(NULL) % 1000;
    strcpy(r.inspector_name, user);
    r.timestamp = time(NULL);
    r.lat = 45.75;
    r.lon = 21.23;
    strcpy(r.category, "road");
    while ((r.severity=rand()%4)==0)
    strcpy(r.description, "Pothole detected.");

    if (write(fd, &r, sizeof(Report)) < 0) {
        perror("write failed");
    }
    close(fd);

    char symlink_name[256];
    sprintf(symlink_name, "active_reports-%s", district);
    unlink(symlink_name);
    if (symlink(filepath, symlink_name) < 0) {
        perror("symlink failed");
    }
    // --- PHASE 2: NOTIFY MONITOR AND LOG ACTION ---
    int notified = 0;
    int pid_fd = open(".monitor_pid", O_RDONLY);

    if (pid_fd >= 0) {
        char pid_buffer[32];
        memset(pid_buffer, 0, sizeof(pid_buffer));
        int bytes_read = read(pid_fd, pid_buffer, sizeof(pid_buffer) - 1);
        close(pid_fd);

        if (bytes_read > 0) {
            pid_t monitor_pid = atoi(pid_buffer);
            if (monitor_pid > 0) {
                // Send SIGUSR1 to the monitor
                if (kill(monitor_pid, SIGUSR1) == 0) {
                    notified = 1;
                }
            }
        }
    }

    // Write to the logged_district file
    char log_filepath[256];
    sprintf(log_filepath, "%s/logged_district", district);

    // Open in append mode, create if missing with 0644 perms
    int log_fd = open(log_filepath, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (log_fd >= 0) {
        char log_msg[512];
        time_t now = time(NULL);

        if (notified) {
            sprintf(log_msg, "[%ld] User: %s | Role: %s | Action: Added Report | Monitor: SUCCESSFULLY NOTIFIED (SIGUSR1)\n",
                    now, user, role);
        } else {
            sprintf(log_msg, "[%ld] User: %s | Role: %s | Action: Added Report | Monitor: FAILED TO NOTIFY (Monitor offline or kill failed)\n",
                    now, user, role);
        }

        write(log_fd, log_msg, strlen(log_msg));
        close(log_fd);
    } else {
        perror("Warning: Could not open logged_district to record action");
    }

    printf("Report added! Monitor notified: %s\n", notified ? "YES" : "NO");
}

void list_reports(const char* district, const char* role, const char* user) {
    char filepath[256];
    sprintf(filepath, "%s/reports.dat", district);

    if (!check_permissions(filepath, role, 0)) {
        printf("Permission denied: Role '%s' cannot read %s\n", role, filepath);
        exit(1);
    }

    struct stat info;
    if (stat(filepath, &info) == 0) {
        char perms[10];
        mode_to_string(info.st_mode, perms);
        printf("File info: %s | Size: %ld | Mod: %s", perms, info.st_size, ctime(&info.st_mtime));
    }

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("open failed");
        return;
    }

    Report r;
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        printf("ID: %d | Cat: %s | Sev: %d | Insp: %s\n",
               r.report_id, r.category, r.severity, r.inspector_name);
    }
    close(fd);
}

void remove_report(const char* district, int report_id, const char* role, const char* user) {
    if (strcmp(role, "manager") != 0) {
        printf("Permission denied: Only managers can remove reports.\n");
        exit(1);
    }

    char filepath[256];
    sprintf(filepath, "%s/reports.dat", district);

    int fd = open(filepath, O_RDWR);
    if (fd < 0) {
        perror("open failed");
        exit(1);
    }

    Report r;
    int index = 0;
    int found = 0;

    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        if (r.report_id == report_id) {
            found = 1;
            break;
        }
        index++;
    }

    if (!found) {
        printf("Report %d not found.\n", report_id);
        close(fd);
        return;
    }

    // Shift subsequent records to the left
    int next_record = index + 1;
    while (1) {
        lseek(fd, next_record * sizeof(Report), SEEK_SET);
        int bytes = read(fd, &r, sizeof(Report));

        if (bytes <= 0) break; // EOF reached

        lseek(fd, (next_record - 1) * sizeof(Report), SEEK_SET);
        write(fd, &r, sizeof(Report));
        next_record++;
    }

    // Truncate file to remove the leftover duplicate at the end
    if (ftruncate(fd, (next_record - 1) * sizeof(Report)) < 0) {
        perror("ftruncate failed");
    } else {
        printf("Report removed.\n");
    }

    close(fd);
}

// AI-Generated functions
int parse_condition(const char *input, char *field, char *op, char *value) {
    if (sscanf(input, "%[^:]:%[^:]:%s", field, op, value) == 3) {
        return 1;
    }
    return 0;
}

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

void filter_reports(const char* district, const char* condition, const char* role, const char* user) {
    char filepath[256];
    sprintf(filepath, "%s/reports.dat", district);

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("open failed");
        return;
    }

    char field[50], op[5], value[50];
    if (!parse_condition(condition, field, op, value)) {
        printf("Invalid filter format. Use field:op:value\n");
        close(fd);
        return;
    }

    Report r;
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        if (match_condition(&r, field, op, value)) {
            printf("MATCH -> ID: %d | Cat: %s | Sev: %d\n", r.report_id, r.category, r.severity);
        }
    }
    close(fd);
}

void update_threshold(const char* district, int value, const char* role, const char* user) {
    // 1. Check if the user is a manager
    if (strcmp(role, "manager") != 0) {
        printf("Permission denied: Only managers can update the threshold.\n");
        exit(1);
    }

    char filepath[256];
    sprintf(filepath, "%s/district.cfg", district);

    struct stat info;

    // 2. Call stat() to get file info
    if (stat(filepath, &info) < 0) {
        // If the file doesn't exist yet, we create it and set the right permissions
        int fd = open(filepath, O_CREAT | O_WRONLY | O_TRUNC, 0640);
        if (fd < 0) {
            perror("open failed");
            exit(1);
        }
        chmod(filepath, 0640); // Force permissions just to be safe

        char buffer[128];
        int len = sprintf(buffer, "severity_threshold=%d\n", value);
        write(fd, buffer, len);
        close(fd);
        printf("Created district.cfg and set threshold to %d.\n", value);
        return;
    }

    // 3. Verify the permission bits match 640 exactly
    // We use & 0777 to isolate the bottom 9 bits (the rwx permissions)
    if ((info.st_mode & 0777) != 0640) {
        printf("Diagnostic Error: Refusing to update! The permissions for %s have been altered (Expected 0640).\n", filepath);
        exit(1);
    }

    // 4. Write the new threshold to the plain-text file
    int fd = open(filepath, O_WRONLY | O_TRUNC);
    if (fd < 0) {
        perror("open failed");
        exit(1);
    }

    char buffer[128];
    int len = sprintf(buffer, "severity_threshold=%d\n", value);
    if (write(fd, buffer, len) < 0) {
        perror("write failed");
    } else {
        printf("Threshold updated successfully to %d.\n", value);
    }

    close(fd);
}

void remove_district(const char* district, const char* role, const char* user) {
    if (strcmp(role, "manager") != 0) {
        printf("Permission denied: Only managers can remove districts.\n");
        exit(1);
    }

    // Danger check: Prevent someone from doing "--remove_district /"
    if (strchr(district, '/') != NULL || strcmp(district, "..") == 0) {
        printf("Error: Invalid district name. Path traversal detected.\n");
        exit(1);
    }

    printf("Removing district '%s' and all its contents...\n", district);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {
        // We are in the child process. Execute rm -rf
        execlp("rm", "rm", "-rf", district, NULL);

        // If execlp succeeds, this line is NEVER reached.
        perror("execlp failed");
        exit(1);
    } else {
        // We are in the parent process. Wait for child to finish deleting.
        int status;
        waitpid(pid, &status, 0);

        // Remove the symlink
        char symlink_name[256];
        sprintf(symlink_name, "active_reports-%s", district);
        unlink(symlink_name);

        printf("District '%s' successfully removed.\n", district);
    }
}