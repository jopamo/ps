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

// oss.c

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "clock.h"
#include "shared.h"

#define MAX_PROCESSES 20

// Process Control Block structure for process table
struct PCB {
  int occupied;      // true or false
  pid_t pid;         // process id
  int startSeconds;  // start time in seconds
  int startNano;     // start time in nanoseconds
};

struct PCB processTable[MAX_PROCESSES];  // Process Table

void spawn_workers( int num_workers, int simul, int timelimit, struct SysClock *sys_clock );
void output_process_table( struct SysClock *sys_clock );
void cleanup_and_exit( int signum );
void handle_error( const char *msg );

int main( int argc, char *argv[] ) {
  // Check for sufficient command-line arguments
  if ( argc < 9 ) {
    fprintf( stderr, "Usage: %s -n proc -s simul -t timelimit -i interval\n", argv[0] );
    return 1;
  }

  // Initialize command-line arguments
  int num_workers = 0;
  int simul       = 0;
  int timelimit   = 0;
  int interval_ms = 0;

  // Parse command-line arguments
  for ( int i = 1; i < argc; i++ ) {
    if ( strcmp( argv[i], "-n" ) == 0 ) {
      num_workers = atoi( argv[++i] );
    } else if ( strcmp( argv[i], "-s" ) == 0 ) {
      simul = atoi( argv[++i] );
    } else if ( strcmp( argv[i], "-t" ) == 0 ) {
      timelimit = atoi( argv[++i] );
    } else if ( strcmp( argv[i], "-i" ) == 0 ) {
      interval_ms = atoi( argv[++i] );
    }
  }

  // Initialize shared memory and semaphore system
  init_shared_memory_system();

  // Create and attach to the shared memory segment for SysClock
  int shmid = create_shared_memory( SHM_KEY, sizeof( struct SysClock ) );
  if ( shmid == -1 ) {
    handle_error( "Failed to create shared memory segment" );
  }

  struct SysClock *sys_clock = attach_shared_memory_rw( shmid );
  if ( sys_clock == NULL ) {
    handle_error( "Failed to attach to shared memory" );
  }

  // Initialize the clock (starts at 0)
  initialize_clock( sys_clock );

  // Set up signal handler for cleanup (after timeout or Ctrl+C)
  setup_signal_handlers();

  // Record start time to calculate elapsed real time
  struct timespec start_time, current_time;
  if ( clock_gettime( CLOCK_MONOTONIC, &start_time ) == -1 ) {
    handle_error( "Failed to get start time" );
  }

  long long real_time_limit_ns = 60ULL * 1000000000ULL;  // 60 seconds real-time limit

  // Track last time output was printed
  long long last_output_time_ns = 0;  // Last time output was printed in nanoseconds

  // Spawn worker processes based on simulated time
  long long last_spawn_time_ns = 0;  // Track the time when the last worker was spawned

  while ( 1 ) {
    // Get current real time
    if ( clock_gettime( CLOCK_MONOTONIC, &current_time ) == -1 ) {
      handle_error( "Failed to get current time" );
    }

    // Calculate the elapsed time in nanoseconds
    long long elapsed_real_time_ns =
      ( current_time.tv_sec - start_time.tv_sec ) * 1000000000LL + ( current_time.tv_nsec - start_time.tv_nsec );

    // Calculate the increment for simulated time based on the time unit and speed factor
    long long increment = (long long)( TIME_UNIT * SPEED_FACTOR );  // Explicitly cast to long long

    // Increment the system clock based on the updated simulated time
    increment_clock( sys_clock, increment );

    // Spawn workers when the simulated time exceeds the spawn interval
    if ( sys_clock->sec * 1000000000LL + sys_clock->nano >= last_spawn_time_ns + interval_ms * 1000000LL ) {
      spawn_workers( num_workers, simul, timelimit, sys_clock );
      last_spawn_time_ns = sys_clock->sec * 1000000000LL + sys_clock->nano;  // Update last spawn time
    }

    // Output the process table every half second (500ms) of simulated time
    long long current_time_ns = sys_clock->sec * 1000000000LL + sys_clock->nano;
    if ( current_time_ns >= last_output_time_ns + 500000000LL ) {  // 500ms in nanoseconds
      output_process_table( sys_clock );
      last_output_time_ns = current_time_ns;  // Update last output time
    }

    // Check for real-time expiration (60 real seconds)
    if ( elapsed_real_time_ns >= real_time_limit_ns ) {
      printf( "Real-time limit reached (60 seconds). Stopping simulated clock.\n" );
      break;
    }
  }

  // Detach shared memory once done
  detach_shared_memory( sys_clock );

  // Clean up shared memory and semaphore system
  cleanup_shared_memory_system();

  return 0;
}

// Spawn worker processes
void spawn_workers( int num_workers, int simul, int timelimit, struct SysClock *sys_clock ) {
  for ( int i = 0; i < num_workers; i++ ) {
    // Add the process to the table
    for ( int j = 0; j < MAX_PROCESSES; j++ ) {
      if ( !processTable[j].occupied ) {
        processTable[j].occupied = 1;
        processTable[j].pid      = fork();
        if ( processTable[j].pid == -1 ) {
          handle_error( "Failed to fork worker process" );
        }

        if ( processTable[j].pid == 0 ) {  // Child process
          // Prepare the arguments as strings
          char simul_str[20], timelimit_str[20];
          snprintf( simul_str, sizeof( simul_str ), "%d", simul );
          snprintf( timelimit_str, sizeof( timelimit_str ), "%d", timelimit );

          // Pass the arguments to the worker
          if ( execlp( "./worker", "worker", simul_str, timelimit_str, (char *)NULL ) == -1 ) {
            handle_error( "Failed to execute worker process" );
          }
          exit( 0 );  // If exec fails, exit child
        }

        processTable[j].startSeconds = sys_clock->sec;
        processTable[j].startNano    = sys_clock->nano;
        break;
      }
    }
  }
}

// Output the current process table
void output_process_table( struct SysClock *sys_clock ) {
  printf( "OSS PID: %d SysClockS: %d SysClockNano: %d\n", getpid(), sys_clock->sec, sys_clock->nano );
  printf( "Process Table:\n" );
  printf( "Entry Occupied PID StartS StartN\n" );

  for ( int i = 0; i < MAX_PROCESSES; i++ ) {
    if ( processTable[i].occupied ) {
      printf( "%d %d %d %d %d\n", i, processTable[i].occupied, processTable[i].pid, processTable[i].startSeconds,
              processTable[i].startNano );
    }
  }
}
