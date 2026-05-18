#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 

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

// A helper struct just for this program to keep track of the math
typedef struct {
    char name[100];
    int total_score;
} InspectorScore;

int main(int argc, char *argv[]) {
    // 1. Check if the hub passed the district name
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <district_name>\n", argv[0]);
        return 1;
    }

    char *district = argv[1];
    char filepath[256];

    // Construct the path to the binary file
    sprintf(filepath, "%s/reports.dat", district);

    // 2. Open the file in binary read mode
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        // If the file/district doesn't exist, print a clean message to the pipe
        printf("--- Workload for District: %s ---\n", district);
        printf("No reports found or district does not exist.\n\n");
        return 0; // Return 0 so it doesn't crash the Hub
    }

    // 3. Create an array to act as our "Scoreboard"
    InspectorScore scoreboard[100];
    int unique_inspectors = 0;
    Report r;

    // 4. Read the binary file chunk by chunk
    while (fread(&r, sizeof(Report), 1, file) == 1) {

        int found = 0;

        // Loop through our scoreboard to see if this inspector is already on it
        for (int i = 0; i < unique_inspectors; i++) {
            if (strcmp(scoreboard[i].name, r.inspector_name) == 0) {
                // We found them! Add the severity to their total
                scoreboard[i].total_score += r.severity;
                found = 1;
                break;
            }
        }

        // If they are not on the scoreboard yet, add them as a new entry
        if (!found && unique_inspectors < 100) {
            strcpy(scoreboard[unique_inspectors].name, r.inspector_name);
            scoreboard[unique_inspectors].total_score = r.severity;
            unique_inspectors++;
        }
    }

    fclose(file);

    // 5. Print the final results (This is what `city_hub` intercepts through the pipe!)
    printf("--- Workload for District: %s ---\n", district);

    if (unique_inspectors == 0) {
        printf("No inspector data available for this district.\n");
    } else {
        for (int i = 0; i < unique_inspectors; i++) {
            // %-15s adds some nice padding so the output looks like a neat table
            printf("Inspector: %-15s | Total Severity Score: %d\n",
                   scoreboard[i].name, scoreboard[i].total_score);
        }
    }
    printf("\n"); // Add a blank line for readability

    return 0;
}