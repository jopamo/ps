#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/*
  Our main executable (oss) will now be maintaining a "simulated system clock" in shared memory.
  This system clock is not tied to the main clock of the system, but instead done separately.
  The clock consists of two separate integers (one storing seconds, the other nanoseconds)
  in shared memory, both of which are initialized to zero.
*/
struct SimulatedClock {
  int seconds;
  int nanoseconds;
};

/*
  In addition to this, oss will also maintain a process table (consisting of Process
  Control Blocks, one for each process). This process table does not need to be in
  shared memory. The first thing it should keep track of is the PID of the child process,
  as well as the time right before oss does a fork to launch that child process (based on our own
  simulated clock).
*/
struct PCB {
  int occupied;     // either true or false
  pid_t pid;        // process id of this child
  int startSeconds; // time when it was forked
  int startNano;    // time when it was forked
};

#define MAX_PROCESSES 20
struct PCB processTable[MAX_PROCESSES];

/*
  The task of oss is to launch a certain number of user processes with particular parameters.
  These numbers are determined by its own command line arguments.
  Your solution will be invoked using the following command:
    oss [-h] [-n proc] [-s simul] [-t timelimitForChildren] [-i intervalInMsToLaunchChildren]
*/
static void print_help(const char *prog)
{
  printf("Usage: %s [-h] [-n proc] [-s simul] [-t timelimitForChildren] [-i intervalInMsToLaunchChildren]\n", prog);
  printf("Your solution will be invoked using the following command:\n");
  printf("    %s [-h] [-n proc] [-s simul] [-t timelimitForChildren] [-i intervalInMsToLaunchChildren]\n", prog);
  printf("\nExample:\n");
  printf("    %s -n 5 -s 2 -t 3 -i 100\n\n", prog);
  printf("If called with the -h parameter, it will show this help message and then terminate.\n");
}

/*
  The -t parameter is different. It now stands for the upper bound of time that a child process
  will be launched for. So for example, if it is called with -t 7, then when calling worker
  processes, it should call them with a time interval randomly between 1 second and 7 seconds
  (with nanoseconds also random).
*/
static int parse_int_arg(const char *prog, const char *arg, char opt_char, int *dest)
{
  if (!arg || arg[0] == '-') {
    fprintf(stderr, "Missing integer after -%c\n", opt_char);
    fprintf(stderr, "Try '%s -h' for usage.\n", prog);
    return -1;
  }
  *dest = atoi(arg);
  if (*dest <= 0) {
    fprintf(stderr, "Invalid value for -%c (must be > 0)\n", opt_char);
    fprintf(stderr, "Try '%s -h' for usage.\n", prog);
    return -1;
  }
  return 0;
}

int parseOptions(int argc, char *argv[], int *n, int *s, int *t, int *i)
{
  int opt;
  opterr = 0;

  while ((opt = getopt(argc, argv, "hn:s:t:i:")) != -1) {
    switch (opt) {
    case 'h': print_help(argv[0]); return 1;
    case 'n':
      if (parse_int_arg(argv[0], optarg, 'n', n) == -1) { return -1; }
      break;
    case 's':
      if (parse_int_arg(argv[0], optarg, 's', s) == -1) { return -1; }
      break;
    case 't':
      if (parse_int_arg(argv[0], optarg, 't', t) == -1) { return -1; }
      break;
    case 'i':
      if (parse_int_arg(argv[0], optarg, 'i', i) == -1) { return -1; }
      break;
    default:
      fprintf(stderr, "Unknown option: -%c\n", optopt);
      fprintf(stderr, "Try '%s -h' for usage.\n", argv[0]);
      return -1;
    }
  }

  if (optind < argc) {
    fprintf(stderr, "Extra non-option argument: %s\n", argv[optind]);
    fprintf(stderr, "Try '%s -h' for usage.\n", argv[0]);
    return -1;
  }

  return 0;
}

#ifndef TESTING
int main(int argc, char *argv[])
{
  int total_children = 0;
  int max_simul = 0;
  int iterations = 0;
  int interval_ms = 0;

  if (parseOptions(argc, argv, &total_children, &max_simul, &iterations, &interval_ms)) { return 0; }

  if (total_children <= 0 || max_simul <= 0 || iterations <= 0 || interval_ms <= 0) {
    fprintf(stderr, "Error: -n, -s, -t, and -i must all be > 0.\n");
    fprintf(stderr, "Try '%s -h' for usage.\n", argv[0]);
    return 1;
  }

  /*
    oss will initialize the system clock and then go into a loop and start doing a fork()
    and then an exec() call to launch worker processes. However, it should only do this up
    to simul number of times.
  */
  key_t key = ftok("oss", 65);
  int shm_id = shmget(key, sizeof(struct SimulatedClock), 0666 | IPC_CREAT);
  struct SimulatedClock *simulated_clock = (struct SimulatedClock *) shmat(shm_id, NULL, 0);
  simulated_clock->seconds = 0;
  simulated_clock->nanoseconds = 0;

  int children_launched = 0;
  int running_children = 0;

  while (children_launched < total_children) {
    while (running_children < max_simul && children_launched < total_children) {
      pid_t pid = fork();
      if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
      }
      else if (pid == 0) {
        /*
          The worker will then attach to shared memory and examine our simulated system clock.
          It will then figure out what time it should terminate by adding up the system clock time
          and the time passed to it (in our simulated system clock, not actual time).
        */
        char iter_str[16];
        snprintf(iter_str, sizeof(iter_str), "%d", iterations);
        execl("./worker", "worker", iter_str, (char *) NULL);
        perror("execl failed");
        exit(EXIT_FAILURE);
      }
      else {
        children_launched++;
        running_children++;
      }
    }
    int status;
    pid_t finished = waitpid(-1, &status, WNOHANG);

    if (finished < 0) {
      perror("wait failed");
      exit(EXIT_FAILURE);
    }
    running_children--;
  }

  /*
    After launching all children, oss will wait for any remaining running children to finish.
    It will ensure that no children are left running before terminating the process.
  */
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
