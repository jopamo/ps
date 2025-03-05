// clock.c

#include "clock.h"
#include <stdio.h>

// Initialize the clock to zero
void initialize_clock(struct SysClock *sys_clock) {
    if (sys_clock) {
        sys_clock->sec = 0;
        sys_clock->nano = 0;
    }
}

// Increment the clock by tick_interval nanoseconds
void increment_clock(struct SysClock *sys_clock, long long tick_interval) {
    if (!sys_clock) return;

    sys_clock->nano += tick_interval;

    // Convert any overflow of nanoseconds into seconds
    while (sys_clock->nano >= 1000000000) {
        sys_clock->nano -= 1000000000;
        sys_clock->sec += 1;
    }
}
