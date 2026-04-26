#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "City_Manager.h"

int main(int argc, char** argv) {
    char *role = NULL;
    char *user = NULL;
    char *command = NULL;
    char *district = NULL;
    char *extra_arg = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0) {
            role = argv[++i];
        } else if (strcmp(argv[i], "--user") == 0) {
            user = argv[++i];
        } else if (strncmp(argv[i], "--", 2) == 0) {
            command = argv[i] + 2;
            if (i + 1 < argc) district = argv[++i];
            if (i + 1 < argc) extra_arg = argv[++i];
        }
    }

    if (role == NULL || user == NULL || command == NULL || district == NULL) {
        printf("Usage error. Make sure to provide --role, --user, and a command.\n");
        exit(1);
    }

    if (strcmp(command, "add") == 0) {
        add_report(district, role, user);
    } else if (strcmp(command, "list") == 0) {
        list_reports(district, role, user);
    } else if (strcmp(command, "remove_report") == 0) {
        if (extra_arg == NULL) {
            printf("Error: Please provide a report ID to remove.\n");
            exit(1);
        }
        remove_report(district, atoi(extra_arg), role, user);
    } else if (strcmp(command, "filter") == 0) {
        if (extra_arg == NULL) {
            printf("Error: Please provide a filter condition.\n");
            exit(1);
        }
        filter_reports(district, extra_arg, role, user);
    } else if (strcmp(command, "update_threshold") == 0) {
        if (extra_arg == NULL) {
            printf("Error: Please provide a threshold value.\n");
            exit(1);
        }
        update_threshold(district, atoi(extra_arg), role, user);
    } else {
        printf("Unknown command: %s\n", command);
    }

    return 0;
}