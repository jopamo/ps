// clock.h

#ifndef CLOCK_H
#define CLOCK_H

/*
 * Simulated clock structure plus function prototypes for incrementing
 * and initializing. This clock is stored in shared memory and updated
 * by the parent (oss). The children (worker) only read it.
 */

// Simple clock struct with seconds + nanoseconds
struct SysClock {
  int sec;   // Seconds
  int nano;  // Nanoseconds
};

// Initializes the clock to zero
void initialize_clock( struct SysClock *sys_clock );

// Increments the clock by a specified amount (in nanoseconds),
// carrying over to 'sec' if needed
void increment_clock( struct SysClock *sys_clock, long long tick_interval );

#endif
