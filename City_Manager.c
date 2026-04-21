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
    r.severity = 2;
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

    printf("Report added successfully!\n");
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