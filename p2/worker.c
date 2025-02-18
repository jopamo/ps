/*
worker, the children

The worker takes in two command line arguments, this time corresponding to the maximum
time it should decide to stay around in the system. For example, if you were running it
directly you might call it like:

    ./worker 5 500000

The worker will then attach to shared memory and examine our simulated system clock. It
will then figure out what time it should terminate by adding up the system clock time and
the time passed to it (in our simulated system clock, not actual time). This is when the
process should decide to leave the system and terminate.

For example, if the system clock was showing 6 seconds and 100 nanoseconds and the worker
was passed 5 and 500000 as above, the target time to terminate in the system would be 11
seconds and 500100 nanoseconds. The worker will then go into a loop, constantly checking
the system clock to see if this time has passed. If it ever looks at the system clock and
sees values over the ones when it should terminate, it should output some information and
then terminate.

So what output should the worker send?

Upon starting up, it should output the following information:

    WORKER PID:6577 PPID:6576 SysClockS: 5 SysclockNano: 1000 TermTimeS: 11 TermTimeNano:
500100
    --Just Starting

The worker should then go into a loop, checking for its time to expire (IT SHOULD NOT DO A
SLEEP). It should also do some periodic output. Every time it notices that the seconds
have changed, it should output a message like:

    WORKER PID:6577 PPID:6576 SysClockS: 6 SysclockNano: 45000000 TermTimeS: 11
TermTimeNano: 500
    --1 seconds have passed since starting

And then one second later it would output:

    WORKER PID:6577 PPID:6576 SysClockS: 7 SysclockNano: 500000 TermTimeS: 11
TermTimeNano: 50010
    --2 seconds have passed since starting

Once its time has elapsed, it would send out one final message:

    WORKER PID:6577 PPID:6576 SysClockS: 11 SysclockNano: 700000 TermTimeS: 11
TermTimeNano: 5001
    --Terminating

API for Shared Memory Operations:
        create_shared_memory(key_t key, size_t size) - Creates and returns a shared memory
        segment with the given key and size. Exits on failure.

        attach_shared_memory_rw(int shmid) - Attaches the shared memory segment (specified
by shmid) with read/write access. Returns a pointer to the attached memory. Mutex
        protection is applied for thread-safety. Exits on failure.

        attach_shared_memory_ro(int shmid) - Attaches the shared memory segment (specified
by shmid) with read-only access. Returns a pointer to the attached memory. Mutex
protection is applied for thread-safety. Exits on failure.

        detach_shared_memory(void* addr) - Detaches the shared memory segment at the given
        address. Mutex protection is applied. Exits on failure.

        init_shared_memory_system(void) - Initializes the shared memory system, including
        semaphores for synchronization.

        cleanup_shared_memory_system(void) - Cleans up the shared memory system, including
        semaphore destruction.
*/

// worker.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "shared.h"

int main( int argc, char *argv[] ) {
  (void)argc;
  (void)argv;

  // Initialize shared memory and semaphore system
  init_shared_memory_system();

  // Step 1: Attach to the shared memory for the system clock with read-only access
  int shmid = create_shared_memory( SHM_KEY, sizeof( struct SysClock ) );

  // Attach to shared memory with read-only access (can't modify shared memory)
  const struct SysClock *clock = (const struct SysClock *)attach_shared_memory_ro( shmid );

  // Step 2: Wait until the system clock reaches a certain point - 5sec
  while ( clock->sec < 5 ) {
#if SHM_DEBUG
    char log_msg[256];
    snprintf( log_msg, sizeof( log_msg ), "Worker PID: %d SysClockS: %d SysClockNano: %d -- Just starting", getpid(),
              clock->sec, clock->nano );
    debug_log( log_msg );
#else
    printf( "Worker PID: %d SysClockS: %d SysClockNano: %d -- Just starting\n", getpid(), clock->sec, clock->nano );
#endif
    sleep( 1 );
  }

// Step 3: Worker exits after 5 seconds
#if SHM_DEBUG
  snprintf( log_msg, sizeof( log_msg ), "Worker PID: %d SysClockS: %d SysClockNano: %d -- Terminating", getpid(),
            clock->sec, clock->nano );
  debug_log( log_msg );
#else
  printf( "Worker PID: %d SysClockS: %d SysClockNano: %d -- Terminating\n", getpid(), clock->sec, clock->nano );
#endif

  // Step 4: Detach shared memory
  detach_shared_memory( (void *)clock );

  // Clean up shared memory and semaphore system
  cleanup_shared_memory_system();

  return 0;
}
