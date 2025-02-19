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
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "clock.h"
#include "shared.h"

int main( int argc, char *argv[] ) {
  if ( argc != 3 ) {
    fprintf( stderr, "Usage: %s <termination_time_sec> <termination_time_ns>\n", argv[0] );
    exit( EXIT_FAILURE );
  }

  // Parse command-line arguments
  int termination_time_sec = atoi( argv[1] );
  int termination_time_ns  = atoi( argv[2] );

  // Attach to shared memory (read-only) to access the simulated clock
  init_shared_memory_system();
  // Create and attach to the shared memory segment for SysClock
  int shmid = create_shared_memory( SHM_KEY, sizeof( struct SysClock ) );
  if ( shmid == -1 ) {
    handle_error( "Failed to create shared memory segment" );
  }
  const struct SysClock *sys_clock = attach_shared_memory_ro( shmid );  // Attach to shared memory (read-only)

  // Calculate target termination time
  int current_sec = sys_clock->sec;
  int current_ns  = sys_clock->nano;

  // Compute the target termination time
  int target_sec = current_sec + termination_time_sec;
  int target_ns  = current_ns + termination_time_ns;

  // Handle nanosecond overflow (if necessary)
  if ( target_ns >= 1000000000 ) {
    target_ns -= 1000000000;
    target_sec += 1;
  }

  // Output the initial worker information
  printf( "WORKER PID:%d PPID:%d SysClockS:%d SysClockNano:%d TermTimeS:%d TermTimeNano:%d\n", getpid(), getppid(),
          sys_clock->sec, sys_clock->nano, target_sec, target_ns );
  printf( "-- Just Starting\n" );

  // Track the last second for periodic output
  int last_sec = sys_clock->sec;

  // Main loop: check the clock to determine when to terminate
  while ( ( sys_clock->sec < target_sec ) || ( sys_clock->sec == target_sec && sys_clock->nano < target_ns ) ) {
    // Periodic output based on clock changes
    if ( sys_clock->sec > last_sec ) {
      printf( "WORKER PID:%d PPID:%d SysClockS:%d SysClockNano:%d TermTimeS:%d TermTimeNano:%d\n", getpid(), getppid(),
              sys_clock->sec, sys_clock->nano, target_sec, target_ns );
      printf( "-- %d seconds have passed since starting\n", sys_clock->sec - last_sec );
      last_sec = sys_clock->sec;
    }
  }

  // Output final message when termination time is reached
  printf( "WORKER PID:%d PPID:%d SysClockS:%d SysClockNano:%d TermTimeS:%d TermTimeNano:%d\n", getpid(), getppid(),
          sys_clock->sec, sys_clock->nano, target_sec, target_ns );
  printf( "-- Terminating\n" );

  // Cleanup: Detach shared memory and clean up resources
  detach_shared_memory( (void *)sys_clock );  // Cast to void* for detaching
  cleanup_shared_memory_system();             // Cleanup shared memory system

  return 0;
}
