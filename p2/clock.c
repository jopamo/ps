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
