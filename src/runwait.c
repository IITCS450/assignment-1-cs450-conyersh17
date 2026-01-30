#define _POSIX_C_SOURCE 199309L
#include "common.h"
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
static void usage(const char *a){fprintf(stderr,"Usage: %s <cmd> [args]\n",a); exit(1);}
static double d(struct timespec a, struct timespec b){
 return (b.tv_sec-a.tv_sec)+(b.tv_nsec-a.tv_nsec)/1e9;}
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        return 1;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        execvp(argv[1], &argv[1]);
        perror("execvp failed");
        exit(1);
    }
    int status;
    waitpid(pid, &status, 0);

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed =
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Child PID: %d\n", pid);

    if (WIFEXITED(status)) {
    int exit_code = WEXITSTATUS(status);   
    printf("Exit Code: %d\n", exit_code);
    printf("exit=%d\n", exit_code);        
} else if (WIFSIGNALED(status)) {
    printf("Terminated by Signal: %d\n", WTERMSIG(status));
}
    printf("Elapsed Time: %.6f seconds\n", elapsed);

    return 0;
}
