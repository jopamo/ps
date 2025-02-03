/*
 * Skeleton multiple processes
 * The user takes in one command line argument. The user executable is never executed directly.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../common/common.h"

int main(int argc, char *argv[]) {
    /* For example, if you were running it directly you would call it like:
       ./user 5
       As it is being called with the number 5, it would do 5 iterations over a loop.
    */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <iterations>\n", argv[0]);
        return 1;
    }

    int iterations = atoi(argv[1]);
    if (iterations <= 0) {
        fprintf(stderr, "Error: iterations must be a positive integer\n");
        return 1;
    }

    pid_t pid = getpid();
    pid_t ppid = getppid();

    for (int i = 1; i <= iterations; i++) {
        printf("USER PID:%d PPID:%d Iteration:%d before sleeping\n", pid, ppid, i);
        fflush(stdout);
        sleep(1);
        printf("USER PID:%d PPID:%d Iteration:%d after sleeping\n", pid, ppid, i);
        fflush(stdout);
    }

    return 0;
}
