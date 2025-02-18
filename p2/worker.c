/*
worker

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
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "shared.h"

int main( int argc, char *argv[] ) {
  if ( argc != 3 ) {
    fprintf( stderr, "Usage: %s <termination_time_sec> <termination_time_ns>\n", argv[0] );
    exit( EXIT_FAILURE );
  }

  // Initialize shared memory and semaphore system
  init_shared_memory_system();

  // Parse command-line arguments for termination time
  int termination_sec = atoi( argv[1] );
  int termination_ns  = atoi( argv[2] );

  // Attach to the shared memory for the system clock with read-only access
  int shmid = create_shared_memory( SHM_KEY, sizeof( struct SysClock ) );

  // Attach to shared memory with read-only access (can't modify shared memory)
  const struct SysClock *clock = (const struct SysClock *)attach_shared_memory_ro( shmid );

  // Calculate the target termination time
  int target_sec = clock->sec + termination_sec;
  int target_ns  = clock->nano + termination_ns;

  if ( target_ns >= 1000000000 ) {
    target_ns -= 1000000000;  // Carry over the nanoseconds to seconds
    target_sec += 1;
  }

  // Output the initial worker information
  printf( "WORKER PID:%d PPID:%d SysClockS:%d SysClockNano:%d TermTimeS:%d TermTimeNano:%d\n", getpid(), getppid(),
          clock->sec, clock->nano, target_sec, target_ns );
  printf( "--Just Starting\n" );

  // Check the system clock until the worker should terminate
  int seconds_passed = 0;
  while ( clock->sec < target_sec || ( clock->sec == target_sec && clock->nano < target_ns ) ) {
    // Check if the seconds have changed and output periodic updates
    if ( clock->sec > seconds_passed ) {
      printf( "WORKER PID:%d PPID:%d SysClockS:%d SysClockNano:%d TermTimeS:%d TermTimeNano:%d\n", getpid(), getppid(),
              clock->sec, clock->nano, target_sec, target_ns );
      printf( "--%d seconds have passed since starting\n", clock->sec );
      seconds_passed = clock->sec;
    }
  }

  // Output final message when the worker terminates
  printf( "WORKER PID:%d PPID:%d SysClockS:%d SysClockNano:%d TermTimeS:%d TermTimeNano:%d\n", getpid(), getppid(),
          clock->sec, clock->nano, target_sec, target_ns );
  printf( "--Terminating\n" );

  // Detach shared memory
  detach_shared_memory( (void *)clock );

  // Clean up shared memory and semaphore system
  cleanup_shared_memory_system();

  return 0;
}
