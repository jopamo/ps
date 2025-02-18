// clock.c

#include "clock.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "shared.h"

// increment the system clock by TICK_INTERVAL
void increment_clock( struct SysClock *sys_clock ) {
  sys_clock->nano += TICK_INTERVAL;  // Increment clock by the defined interval

  // Handle overflow of nanoseconds into seconds
  if ( sys_clock->nano >= 1000000000 ) {  // 1 second = 1,000,000,000 nanoseconds
    sys_clock->nano -= 1000000000;        // Subtract 1 second worth of nanoseconds
    sys_clock->sec += 1;                  // Increment seconds
  }
}

// initialize the clock and set it to 0
void initialize_clock( struct SysClock *sys_clock ) {
  sys_clock->sec  = 0;
  sys_clock->nano = 0;
}

// start the clock increment loop
void start_clock_loop( struct SysClock *sys_clock ) {
  clock_t start_time = clock();  // Record the initial clock time

  while ( 1 ) {
    clock_t current_time = clock();  // Get the current clock time

    // Calculate elapsed time in nanoseconds
    long elapsed_time_ns = ( current_time - start_time ) * ( 1000000000 / CLOCKS_PER_SEC );

    // Check if the elapsed time has reached the TICK_INTERVAL
    if ( elapsed_time_ns >= TICK_INTERVAL ) {
      increment_clock( sys_clock );  // Increment the system clock

// Print the current system clock values
#if SHM_DEBUG
      char log_msg[256];
      snprintf( log_msg, sizeof( log_msg ), "OSS PID: %d SysClockS: %d SysClockNano: %d", getpid(), sys_clock->sec,
                sys_clock->nano );
      debug_log( log_msg );
#else
      printf( "OSS PID: %d SysClockS: %d SysClockNano: %d\n", getpid(), sys_clock->sec, sys_clock->nano );
#endif

      start_time = current_time;  // Reset start time for next interval
    }
  }
}
