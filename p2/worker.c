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
 *      - `startSeconds` and `startNano` (time when the process was launched based on the system clock).
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
 *      - -s simul: simulated time for each worker process
 *      - -t timelimitForChildren: upper bound of time for workers
 *      - -i intervalInMsToLaunchChildren: interval between worker launches
 *    - Initializes the system clock and launches workers up to the specified limit.
 *    - Updates the process table as processes are launched and terminated.
 *    - Increments the system clock, outputs the process table every half second.
 *    - Non-blocking `wait()` is used to check if child processes have terminated.
 *    - Must ensure the program runs for no more than 60 real seconds using a timeout signal.
 *
 * 5. Termination:
 *    - After 60 real seconds, send a signal to terminate all running child processes.
 *    - Handle `SIGINT` (Ctrl-C) to clean up shared memory and terminate children.
 *
 * Notes:
 * - Do NOT use `sleep()` or `usleep()` for delays based on real time.
 * - The system clock does not have to match real time precisely, but should behave similarly.
 * - Handle clock race conditions by allowing minor discrepancies in worker termination.
 *
 * Signal handling and termination are critical for ensuring correct program behavior.
 */

// worker.c

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "clock.h"
#include "shared.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: worker <sec_to_live> <nano_to_live>\n");
        return 1;
    }

    // parse
    int sec_to_live  = atoi(argv[1]);
    int nano_to_live = atoi(argv[2]);

    // Setup shared memory system for the child as well (open semaphore)
    init_shared_memory_system();

    // Attach to the existing SysClock in read-only mode
    int shmid = create_shared_memory(SHM_KEY, sizeof(struct SysClock));
    const struct SysClock *sys_clock =
        (const struct SysClock *)attach_shared_memory_ro(shmid);
    if (!sys_clock) {
        perror("worker attach");
        return 1;
    }

    // current time
    int start_sec  = sys_clock->sec;
    int start_nano = sys_clock->nano;

    // compute target
    int end_sec  = start_sec  + sec_to_live;
    int end_nano = start_nano + nano_to_live;
    while (end_nano >= 1000000000) {
        end_nano -= 1000000000;
        end_sec++;
    }

    // Print start message
    printf("WORKER PID:%d Start: %d s, %d ns -> End: %d s, %d ns\n",
           getpid(), start_sec, start_nano, end_sec, end_nano);

    int last_reported_sec = start_sec;

    // loop until time >= end_time
    while (1) {
        int current_s  = sys_clock->sec;
        int current_ns = sys_clock->nano;

        // if we've reached or passed the target time, break
        if (current_s > end_sec ||
            (current_s == end_sec && current_ns >= end_nano)) {
            printf("WORKER PID:%d terminating at %d s, %d ns\n",
                   getpid(), current_s, current_ns);
            break;
        }

        // every time we cross a new second, output a quick message
        if (current_s > last_reported_sec) {
            printf("WORKER PID:%d alive for %d seconds\n",
                   getpid(), (current_s - start_sec));
            last_reported_sec = current_s;
        }
    }

    // cleanup
    detach_shared_memory((void *)sys_clock);
    cleanup_shared_memory_system();

    return 0;
}
