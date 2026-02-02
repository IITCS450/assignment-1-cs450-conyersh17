#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 4096

int is_number(const char *s) {
    for (int i = 0; s[i]; i++) {
        if (!isdigit((unsigned char)s[i])) return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return 1;
    }

    if (!is_number(argv[1])) {
        fprintf(stderr, "Error: PID must be numeric\n");
        return 1;
    }

    int pid = atoi(argv[1]);
    char path[256], line[MAX_LINE];
    FILE *fp;
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    fp = fopen(path, "r");
    if (!fp) {
        perror("Error opening stat file");
        return 1;
    }
    if (!fgets(line, sizeof(line), fp)) {
        perror("Error reading stat file");
        fclose(fp);
        return 1;
    }
    fclose(fp);

    char comm[256];
    char state;
    int ppid;
    int num = sscanf(line, "%d (%[^)]) %c %d", &pid, comm, &state, &ppid);
    if (num != 4) {
        fprintf(stderr, "Failed to parse stat file header\n");
        return 1;
    }
    char *ptr = strchr(line, ')');
    if (!ptr) {
        fprintf(stderr, "Malformed stat file\n");
        return 1;
    }
    ptr += 2; 
    char *saveptr;
    char *token = ptr;
    int i;

    for (i = 4; i <= 13; i++) {
        token = strtok_r(token, " ", &saveptr);
        if (!token) {
            fprintf(stderr, "Malformed stat file (skipping fields)\n");
            return 1;
        }
        token = NULL; 
    }
    token = strtok_r(NULL, " ", &saveptr);
    if (!token) {
        fprintf(stderr, "Failed to get utime\n");
        return 1;
    }
    unsigned long utime_ticks = strtoul(token, NULL, 10);

    token = strtok_r(NULL, " ", &saveptr);
    if (!token) {
        fprintf(stderr, "Failed to get stime\n");
        return 1;
    }
    unsigned long stime_ticks = strtoul(token, NULL, 10);
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    fp = fopen(path, "r");
    if (!fp) {
        perror("Error opening status file");
        return 1;
    }

    long vmrss = -1;
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS: %ld", &vmrss);
            break;
        }
    }
    fclose(fp);

    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    fp = fopen(path, "r");
    if (!fp) {
        perror("Error opening cmdline file");
        return 1;
    }
    char cmdline[MAX_LINE];
    size_t len = fread(cmdline, 1, sizeof(cmdline) - 1, fp);
    fclose(fp);

    if (len == 0) {
        strcpy(cmdline, "[kernel process]");
    } else {
        for (size_t i = 0; i < len - 1; i++) {
            if (cmdline[i] == '\0')
                cmdline[i] = ' ';
        }
        cmdline[len] = '\0';
    }

    long ticks_per_sec = sysconf(_SC_CLK_TCK);
    double cpu_time_sec = (utime_ticks + stime_ticks) / (double)ticks_per_sec;

    printf("PID: %d\n", pid);
    printf("State: %c\n", state);
    printf("Parent PID: %d\n", ppid);
    printf("Command Line: %s\n", cmdline);
    printf("CPU Time: %.2f seconds\n", cpu_time_sec);
    if (vmrss != -1)
        printf("Resident Memory (VmRSS): %ld kB\n", vmrss);
    else
        printf("Resident Memory (VmRSS): N/A\n");

    return 0;
}
