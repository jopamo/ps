// clock.c

#include "clock.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "shared.h"

// Increment the system clock by the defined tick interval
void increment_clock( struct SysClock *sys_clock, long long tick_interval ) {
  sys_clock->nano += tick_interval;  // Add tick to nanoseconds

  // Handle overflow of nanoseconds into seconds
  if ( sys_clock->nano >= 1000000000 ) {  // 1 second = 1,000,000,000 nanoseconds
    sys_clock->nano -= 1000000000;        // Subtract one second
    sys_clock->sec += 1;                  // Increment seconds
  }
}

void initialize_clock( struct SysClock *sys_clock ) {
  sys_clock->sec  = 0;
  sys_clock->nano = 0;
}

// Start the clock loop and stop after 60 real-time seconds
void start_clock_loop( struct SysClock *sys_clock ) {
  struct timespec start_time, current_time;
  clock_gettime( CLOCK_MONOTONIC, &start_time );  // Record the real-time start

  long long simulated_time_ns  = 0;                      // Track simulated time in nanoseconds
  long long real_time_limit_ns = 60ULL * 1000000000ULL;  // 60 seconds real-time limit

  while ( 1 ) {
    // Get current real time
    clock_gettime( CLOCK_MONOTONIC, &current_time );

    // Calculate the elapsed time in nanoseconds
    long long elapsed_real_time_ns =
      ( current_time.tv_sec - start_time.tv_sec ) * 1000000000LL + ( current_time.tv_nsec - start_time.tv_nsec );

    // Calculate the difference between real-time and simulated time
    long long real_time_diff = elapsed_real_time_ns - simulated_time_ns;

    // Calculate the increment for simulated time based on the time unit and speed factor
    long long increment = (long long)( TIME_UNIT * SPEED_FACTOR );  // Explicitly cast to long long

    // Adjust the simulated time
    if ( real_time_diff > 0 ) {
      // Gradually increase simulated time by the adjusted increment
      simulated_time_ns += increment;
    } else if ( real_time_diff < 0 ) {
      // Gradually decrease
      simulated_time_ns += increment;
    } else {
      simulated_time_ns += increment;  // Normal increment with speed factor
    }

    // Increment the system clock based on the updated simulated time
    increment_clock( sys_clock, increment );

    // Output the clock value every second of simulated time
    // if ( sys_clock->nano >= 100000000 ) {  // Every second of simulated time
    //  printf( "OSS PID: %d SysClockS: %d SysClockNano: %d\n", getpid(), sys_clock->sec, sys_clock->nano );
    //}

    // Check for real-time expiration (60 real seconds)
    if ( elapsed_real_time_ns >= real_time_limit_ns ) {
      printf( "Real-time limit reached (60 seconds). Stopping simulated clock.\n" );
      break;
    }
  }
}
