#ifndef SO_CITY_MANAGER_H
#define SO_CITY_MANAGER_H

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>

typedef struct {
    int report_id;
    char inspector_name[100];
    float lat;
    float lon;
    char category[100];
    int severity; // 1 = minor, 2 = moderate, 3 = critical
    time_t timestamp;
    char description[200];
} Report;

void mode_to_string(mode_t mode, char* str);
int check_permissions(const char* filepath, const char* role, int check_write);
void add_report(const char* district, const char* role, const char* user);
void list_reports(const char* district, const char* role, const char* user);
void remove_report(const char* district, int report_id, const char* role, const char* user);
void filter_reports(const char* district, const char* condition, const char* role, const char* user);
void update_threshold(const char* district, int value, const char* role, const char* user);
void remove_district(const char* district, const char* role, const char* user);

// AI-Assisted functions
int parse_condition(const char *input, char *field, char *op, char *value);
int match_condition(Report *r, const char *field, const char *op, const char *value);

#endif //SO_CITY_MANAGER_H