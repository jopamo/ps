// clock.h

#ifndef CLOCK_H
#define CLOCK_H

#include "shared.h"

// Tweakable value for clock increments (in nanoseconds)
#define TICK_INTERVAL 100000000  // 100ms (100,000,000 nanoseconds)

void increment_clock( struct SysClock *sys_clock );
void initialize_clock( struct SysClock *sys_clock );
void start_clock_loop( struct SysClock *sys_clock );

#endif  // CLOCK_H
