/*
 * Project: Adding Functionality to Our System
 * Purpose: This program simulates a system with a custom clock and manages worker processes.
 *
 * Overview:
 * - Two executables: `oss` (parent) and `worker` (child).
 * - `oss` manages a simulated system clock, process table, and launches worker processes.
 * - Worker processes run based on the simulated system clock and terminate after a specified time.
 *
 * Details:
 * 1. Simulated System Clock:
 *    - Stored in shared memory (seconds and nanoseconds).
 *    - Accessible by worker processes but not modified by them.
 *    - Initialized to zero.
 *
 * 2. Process Table:
 *    - Stores Process Control Blocks (PCBs) for each worker process.
 *    - Each PCB contains:
 *      - `occupied` (0 or 1) to indicate if the entry is in use.
 *      - `pid` (process ID of the child).
 *      - `startSec` and `startNano` (time when the process was launched based on the system clock).
 *    - Limited size (suggested 20 entries).
 *    - Old entries are reused after processes terminate.
 *
 * 3. Worker Processes:
 *    - Takes two command-line arguments: maximum time to run (seconds and nanoseconds).
 *    - Attach to shared memory to access the simulated clock.
 *    - Compute the target termination time by adding its runtime to the system clock.
 *    - Periodically check the system clock and output progress.
 *    - Terminate when the target time has passed.
 *
 * 4. Parent Process (oss):
 *    - Launches worker processes based on user-defined parameters.
 *    - Arguments:
 *      - -h: help
 *      - -n proc: number of worker processes to launch
 *      - -s simul: concurrency limit (maximum processes running simultaneously)
 *      - -t timelimit: how many simulated seconds a child can live
 *      - -i intervalInMs: how many simulated ms between spawns
 *    - Uses a busy loop to increment the simulated clock, printing status every 0.5s, respecting concurrency,
 *      and checking for child termination non-blockingly.
 *    - Kills all children and cleans up if 60 real seconds pass.
 *
 * 5. Termination:
 *    - After 60 real seconds, send a signal to terminate all running child processes.
 *    - Handle `SIGINT` (Ctrl-C) to clean up shared memory and terminate children.
 *
 * Notes:
 * - No `sleep()` or `usleep()` used for time delays.
 * - The system clock can diverge from real time, but we try to keep it close by adapting the increment.
 * - Minor discrepancies are acceptable for child termination timing.
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "clock.h"
#include "shared.h"

#define MAX_PROCESSES 20

// We'll stop after 60 real seconds
#define REAL_TIME_LIMIT_SEC 60

// Print the process table every 0.5 simulated seconds
#define HALF_SECOND_NS 500000000LL

/*
 * ADAPTIVE TICK PARAMETERS:
 * - We do a busy loop, each iteration incrementing our simulated clock by 'current_increment'.
 * - Then, every FEEDBACK_CHECK_INTERVAL loops, we measure:
 *       (sim_time_passed / real_time_passed)
 *   If ratio>1 => sim is running faster => reduce increment
 *   If ratio<1 => sim is slower => increase increment
 */

// Adjust these to taste:
#define INITIAL_INCREMENT_NS    50000LL   // 0.05 ms
#define FEEDBACK_CHECK_INTERVAL 500        // Check ratio every 500 loop iterations
#define ADJUSTMENT_FACTOR       0.1        // Base factor for adjusting increments
#define MIN_INCREMENT           5000LL
#define MAX_INCREMENT           2000000LL // 2 ms

// We'll also use a dead band around 1.0 speed to avoid small jitter
#define DEAD_BAND_LOWER 0.95
#define DEAD_BAND_UPPER 1.05

// We can clamp single-step changes to e.g. ±25% of current increment
#define MAX_SINGLE_STEP_RATIO 0.25

/*
 * SPIN_COUNT => how many dummy iterations to run each loop
 * A higher SPIN_COUNT means the loop uses more CPU time per iteration,
 * slowing down your program in *real* time without calling sleep().
 */
#define SPIN_COUNT 5000

// PCB struct for each worker
struct PCB {
    int occupied;  // 1 = in use, 0 = free
    pid_t pid;     // child's PID
    int startSec;  // time (seconds) in the simulation when forked
    int startNano; // time (nanoseconds) in the simulation when forked
};

static struct PCB processTable[MAX_PROCESSES];

// We'll store the SysClock pointer after attaching shared memory
static struct SysClock *sys_clock = NULL;
// We'll store the shared memory ID
static int shmid = -1;

// Command line args
static int num_workers = 0;  // -n
static int simul       = 0;  // -s
static int timelimit   = 0;  // -t
static int interval_ms = 0;  // -i

// How many total workers have been launched
static int launched_count = 0;

// We'll track the real time at start to enforce the 60-second limit
static struct timespec real_start;

// The "adaptive" increment
static long long current_increment = INITIAL_INCREMENT_NS;
static int iteration_count = 0; // loop iteration count

// For measuring how sim time compares to real time
static struct timespec feedback_real_start;
static long long feedback_sim_start_ns = 0; // baseline sim time for feedback

// Prototypes
static void parse_args(int argc, char *argv[]);
static void spawn_one_worker(void);
static void handle_nonblocking_wait(void);
static void print_process_table(void);
static void kill_all_children(void);
static void cleanup_and_exit(void);

int main(int argc, char *argv[]) {
    parse_args(argc, argv);

    // Clear out the process table
    memset(processTable, 0, sizeof(processTable));

    // 1) Initialize semaphore / shared memory system
    init_shared_memory_system();

    // 2) Create & attach the SysClock
    shmid = create_shared_memory(SHM_KEY, sizeof(struct SysClock));
    sys_clock = (struct SysClock *)attach_shared_memory_rw(shmid);
    if (!sys_clock) {
        handle_error("Failed to attach shared memory (RW)");
    }

    // 3) Initialize the clock to zero
    initialize_clock(sys_clock);

    // 4) Capture real start time for the 60s cutoff
    if (clock_gettime(CLOCK_MONOTONIC, &real_start) == -1) {
        perror("clock_gettime (start)");
        cleanup_and_exit();
    }

    // Initialize feedback baseline
    feedback_real_start = real_start;
    feedback_sim_start_ns = 0; // we just zeroed the clock

    long long last_print_ns = 0; // for printing table every 0.5s
    long long last_spawn_ns = 0; // track last spawn time in sim ns

    // Main loop
    while (1) {
        // (A) Check if 60 real seconds have passed
        struct timespec now;
        if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
            perror("clock_gettime (loop)");
            cleanup_and_exit();
        }
        long long elapsed_real_ns =
            (long long)(now.tv_sec - real_start.tv_sec) * 1000000000LL +
            (long long)(now.tv_nsec - real_start.tv_nsec);

        if (elapsed_real_ns >= (long long)REAL_TIME_LIMIT_SEC * 1000000000LL) {
            printf("OSS: 60 real seconds elapsed. Stopping.\n");
            break;
        }

        // (B) ***Spin*** to slow down the loop in real time
        for (volatile int i = 0; i < SPIN_COUNT; i++) {
            // do nothing - purely burn CPU time
        }

        // (C) Increment the simulated clock by current_increment
        increment_clock(sys_clock, current_increment);

        // (D) Check for finished children (non-blocking wait)
        handle_nonblocking_wait();

        // (E) Possibly spawn a new worker if concurrency & interval allow
        if (launched_count < num_workers) {
            // Count active
            int active_count = 0;
            for (int i = 0; i < MAX_PROCESSES; i++) {
                if (processTable[i].occupied) active_count++;
            }
            if (active_count < simul) {
                // If we've advanced enough sim time since last spawn
                long long sim_now_ns =
                    (long long)sys_clock->sec * 1000000000LL + sys_clock->nano;
                if (sim_now_ns >= last_spawn_ns + (long long)interval_ms * 1000000LL) {
                    spawn_one_worker();
                    launched_count++;
                    last_spawn_ns = sim_now_ns;
                }
            }
        }

        // (F) Print table every 0.5 sim seconds
        long long current_sim_ns =
            (long long)sys_clock->sec * 1000000000LL + sys_clock->nano;
        if (current_sim_ns >= last_print_ns + HALF_SECOND_NS) {
            print_process_table();
            last_print_ns = current_sim_ns;
        }

        // (G) If all workers launched & none active => done
        if (launched_count >= num_workers) {
            int still_active = 0;
            for (int i = 0; i < MAX_PROCESSES; i++) {
                if (processTable[i].occupied) still_active++;
            }
            if (still_active == 0) {
                printf("OSS: All workers finished.\n");
                break;
            }
        }

        // (H) Every FEEDBACK_CHECK_INTERVAL loops, measure ratio & adapt
        iteration_count++;
        if (iteration_count % FEEDBACK_CHECK_INTERVAL == 0) {
            // measure real time since last feedback
            struct timespec now_fb;
            if (clock_gettime(CLOCK_MONOTONIC, &now_fb) == -1) {
                perror("clock_gettime (feedback)");
                cleanup_and_exit();
            }
            long long real_passed_ns =
                (long long)(now_fb.tv_sec - feedback_real_start.tv_sec) * 1000000000LL +
                (long long)(now_fb.tv_nsec - feedback_real_start.tv_nsec);

            // measure how much simulated time advanced
            long long sim_now_ns2 =
                (long long)sys_clock->sec * 1000000000LL + sys_clock->nano;
            long long sim_passed_ns = sim_now_ns2 - feedback_sim_start_ns;

            double ratio = 0.0;
            if (real_passed_ns > 0) {
                ratio = (double)sim_passed_ns / (double)real_passed_ns;
            }

            // If ratio ~ 1 => no change
            if (ratio < DEAD_BAND_LOWER || ratio > DEAD_BAND_UPPER) {
                // out of dead band => let's adapt
                double error = ratio - 1.0;

                // 1) Compute how much to shift based on ADJUSTMENT_FACTOR and error
                double dbl_product1 = (double)current_increment * ADJUSTMENT_FACTOR * error;
                long long delta = (long long)dbl_product1;

                // 2) clamp single-step changes to ±(25%) of current_increment
                double dbl_product2 = (double)current_increment * MAX_SINGLE_STEP_RATIO;
                long long maxChange = (long long)dbl_product2;

                if (delta > maxChange) {
                    delta = maxChange;
                } else if (delta < -maxChange) {
                    delta = -maxChange;
                }

                // 3) Apply
                current_increment -= delta;

                // 4) Bound the increment in [MIN_INCREMENT, MAX_INCREMENT]
                if (current_increment < MIN_INCREMENT) {
                    current_increment = MIN_INCREMENT;
                } else if (current_increment > MAX_INCREMENT) {
                    current_increment = MAX_INCREMENT;
                }
            }
            // else do nothing if ratio is within the dead band

            // reset feedback baseline
            feedback_real_start = now_fb;
            feedback_sim_start_ns = sim_now_ns2;
        }

        // busy loop => no other real sleeps
    }

    // done => cleanup
    cleanup_and_exit();
    return 0;
}

// ------------------------------------------------------------------------
static void parse_args(int argc, char *argv[]) {
    if (argc < 9) {
        fprintf(stderr, "Usage: %s -n <num_workers> -s <simul> -t <timelimit> -i <interval_ms>\n", argv[0]);
        exit(1);
    }
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            num_workers = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-s") == 0) {
            simul = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0) {
            timelimit = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-i") == 0) {
            interval_ms = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s -n <num_workers> -s <simul> -t <timelimit> -i <interval_ms>\n", argv[0]);
            exit(0);
        }
    }
}

// ------------------------------------------------------------------------
static void spawn_one_worker(void) {
    // find a free PCB
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!processTable[i].occupied) {
            processTable[i].occupied = 1;
            processTable[i].startSec  = sys_clock->sec;
            processTable[i].startNano = sys_clock->nano;

            pid_t cpid = fork();
            if (cpid < 0) {
                perror("fork");
                processTable[i].occupied = 0;
                return;
            }
            if (cpid == 0) {
                // Child
                // We'll pass timelimit as <sec> plus 500000000 ns
                char sec_str[32], ns_str[32];
                snprintf(sec_str, sizeof(sec_str), "%d", timelimit);
                snprintf(ns_str, sizeof(ns_str), "%d", 500000000);

                execlp("./worker", "worker", sec_str, ns_str, (char *)NULL);
                perror("execlp worker");
                exit(1);
            }
            // parent
            processTable[i].pid = cpid;
            return;
        }
    }
    fprintf(stderr, "OSS: No free slot in processTable.\n");
}

// ------------------------------------------------------------------------
static void handle_nonblocking_wait(void) {
    int status;
    pid_t cpid;
    while ((cpid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Mark that PCB slot free
        for (int i = 0; i < MAX_PROCESSES; i++) {
            if (processTable[i].occupied && processTable[i].pid == cpid) {
                processTable[i].occupied = 0;
                break;
            }
        }
    }
}

// ------------------------------------------------------------------------
static void print_process_table(void) {
    printf("\nOSS: SysClock %d s, %d ns, incr=%lld\n",
           sys_clock->sec, sys_clock->nano, current_increment);
    printf("Process Table (PID / startSec / startNano):\n");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processTable[i].occupied) {
            printf("  [%2d] pid=%d start=(%d, %d)\n",
                   i,
                   processTable[i].pid,
                   processTable[i].startSec,
                   processTable[i].startNano);
        }
    }
    printf("\n");
}

// ------------------------------------------------------------------------
static void kill_all_children(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processTable[i].occupied) {
            kill(processTable[i].pid, SIGTERM);
        }
    }
    // Final reap
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) { }
}

// ------------------------------------------------------------------------
static void cleanup_and_exit(void) {
    kill_all_children();

    if (sys_clock) {
        detach_shared_memory((void *)sys_clock);
    }
    if (shmid != -1) {
        cleanup_shared_memory(shmid);
    }
    cleanup_shared_memory_system();

    exit(0);
}
