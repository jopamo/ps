/*
 "The task of oss is to launch a certain number of user processes with particular parameters.
  These numbers are determined by its own command line arguments."
 "Your solution will be invoked using the following command:
  oss [-h] [-n proc] [-s simul] [-t iter]"
 "If called with the -h parameter, it should simply output a help message (indicating how it is supposed to be run)
  and then terminating."
 "Implement oss to fork() and then exec() off one user to do its task and wait() until it is done. [Day 5]"
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "../common/common.h"

static void print_help(const char *prog) {
    printf("Usage: %s [-h] [-n proc] [-s simul] [-t iter]\n", prog);
}

int main(int argc, char *argv[]) {
    int total_children = 0;
    int max_simul = 0;
    int iterations = 0;
    int opt;

    while ((opt = getopt(argc, argv, "hn:s:t:")) != -1) {
        switch (opt) {
            case 'h':
                print_help(argv[0]);
                return 0;
            case 'n':
                total_children = atoi(optarg);
                break;
            case 's':
                max_simul = atoi(optarg);
                break;
            case 't':
                iterations = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Unknown option\n");
                return 1;
        }
    }

    /*
     "So now that I know what parameters it should run with, what should it do?
      oss when launched should go into a loop and start doing a fork() and then an exec() call
      to launch user processes. However, it should only do this up to simul number of times."
      [But here we only do one fork() and exec() for Day 5 demonstration.]
    */
    if (total_children <= 0 || iterations <= 0) {
        fprintf(stderr, "Invalid or missing -n and -t\n");
        return 1;
    }

    // Only one user for demonstration (Day 5)
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        char iter_str[16];
        snprintf(iter_str, sizeof(iter_str), "%d", iterations);
        execl("./p1/user", "user", iter_str, (char *)NULL);
        perror("execl failed");
        exit(EXIT_FAILURE);
    } else {
        int status;
        if (wait(&status) < 0) {
            perror("wait failed");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
