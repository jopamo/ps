/*
 * Skeleton multiple processes
 * The task of oss is to launch a certain number of user processes with particular parameters.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../common/common.h"

static void print_help(const char *prog) {
    /* If called with the -h parameter, it should simply output a help message and then terminate. */
    printf("Usage: %s [-h] [-n proc] [-s simul] [-t iter]\n", prog);
}

int main(int argc, char *argv[]) {
    /* The proc parameter stands for number of total children to launch,
       The simul parameter indicates how many children to allow to run simultaneously,
       iter is the number to pass to the user process. */
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

    /* (Parsing only at this point) */
    printf("Parameters: -n %d, -s %d, -t %d\n", total_children, max_simul, iterations);
    return 0;
}
