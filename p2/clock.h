// clock.h

#ifndef CLOCK_H
#define CLOCK_H

#include "shared.h"

#define TIME_UNIT 10
#define SPEED_FACTOR 2.68  // Speed factor (1 = normal, >1 = faster, <1 = slower)

void increment_clock( struct SysClock *sys_clock, long long tick_interval );
void initialize_clock( struct SysClock *sys_clock );
void start_clock_loop( struct SysClock *sys_clock );

#endif  // CLOCK_H
