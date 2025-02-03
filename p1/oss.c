/*
 The task of oss is to launch a certain number of user processes with
 particular parameters. These numbers are determined by its own command line arguments.
 Your solution will be invoked using the following command:
    oss [-h] [-n proc] [-s simul] [-t iter]
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 If called with the -h parameter, it should simply output a help message
  (indicating how it is supposed to be run) and then terminating.
*/
static void print_help(const char *prog)
{
    printf("Usage: %s [-h] [-n proc] [-s simul] [-t iter]\n", prog);
}

int parseOptions(int argc, char *argv[], int *n, int *s, int *t)
{
    int opt;
    opterr = 0;

    while ((opt = getopt(argc, argv, "hn:s:t:")) != -1) {
        switch (opt) {
        case 'h':
            print_help(argv[0]);
            return 1;
        case 'n':
            if (!optarg || optarg[0] == '-') {
                fprintf(stderr, "Missing integer after -n\n");
                return -1;
            }
            *n = atoi(optarg);
            if (*n <= 0) {
                fprintf(stderr, "Invalid value for -n (must be > 0)\n");
                return -1;
            }
            break;
        case 's':
            if (!optarg || optarg[0] == '-') {
                fprintf(stderr, "Missing integer after -s\n");
                return -1;
            }
            *s = atoi(optarg);
            if (*s <= 0) {
                fprintf(stderr, "Invalid value for -s (must be > 0)\n");
                return -1;
            }
            break;
        case 't':
            if (!optarg || optarg[0] == '-') {
                fprintf(stderr, "Missing integer after -t\n");
                return -1;
            }
            *t = atoi(optarg);
            if (*t <= 0) {
                fprintf(stderr, "Invalid value for -t (must be > 0)\n");
                return -1;
            }
            break;
        default:
            fprintf(stderr, "Unknown option: %c\n", optopt ? optopt : '-');
            return -1;
        }
    }

    if (optind < argc) {
        fprintf(stderr, "Extra non-option argument: %s\n", argv[optind]);
        return -1;
    }

    return 0;
}

#ifndef TESTING
int main(int argc, char *argv[])
{
    /*
     So now that I know what parameters it should run with, what should it do?
      -oss when launched should go into a loop
      -start doing a fork() and then an exec() call to launch user processes
      -it should only do this up to simul number of times.
    */
    int total_children = 0;
    int max_simul = 0;
    int iterations = 0;

    if (parseOptions(argc, argv, &total_children, &max_simul, &iterations)) {
        return 0;
    }

    if (total_children <= 0 || max_simul <= 0 || iterations <= 0) {
        fprintf(stderr, "Error: -n, -s, -t must all be > 0\n");
        return 1;
    }

    int children_launched = 0;
    int running_children = 0;

    while (children_launched < total_children) {
        while (running_children < max_simul && children_launched < total_children) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork failed");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                char iter_str[16];
                snprintf(iter_str, sizeof(iter_str), "%d", iterations);
                execl("./user", "user", iter_str, (char *) NULL);
                perror("execl failed");
                exit(EXIT_FAILURE);
            } else {
                children_launched++;
                running_children++;
            }
        }
        {
            int status;
            pid_t finished = wait(&status);
            if (finished < 0) {
                perror("wait failed");
                exit(EXIT_FAILURE);
            }
            running_children--;
        }
    }

    while (running_children > 0) {
        int status;
        if (wait(&status) < 0) {
            perror("wait failed");
            exit(EXIT_FAILURE);
        }
        running_children--;
    }

    return 0;
}
#endif
