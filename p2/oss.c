/*
oss.c

worker.c should be launched as a seprate program
./worker 5 500000

Task:
As in the previous project, you will have two executables, oss and worker. The oss
executable will be launching workers, who stick around for a bit. However, we have a few
more details, the most important of which is that our processes stick around based on a
simulated clock that we create, NOT system time. IMPORTANT: Your code should not call
sleep or usleep or any related function that waits based on some amount of system time.

Our main executable (oss) will now be maintaining a "simulated system clock" in shared
memory. This system clock is not tied to the main clock of the system, but instead done
separately. The clock consists of two separate integers (one storing seconds, the other
nanoseconds) in shared memory, both of which are initialized to zero. This system clock
must be accessible by the children, so it is required to be in shared memory. The children
will not be modifying this clock for this assignment, but they will need to look at it.

In addition to this, oss will also maintain a process table (consisting of Process Control
Blocks, one for each process). This process table does not need to be in shared memory.
The first thing it should keep track of is the PID of the child process, as well as the
time right before oss does a fork to launch that child process (based on our own simulated
clock). It should also contain an entry for if this entry in the process table is empty
(i.e., not being used).

I suggest making your process table an array of structs of PCBs, for example:

struct PCB {
    int occupied;     // either true or false
    pid_t pid;        // process id of this child
    int startSeconds; // time when it was forked
    int startNano;    // time when it was forked
};

struct PCB processTable[20];

Note that your process table is limited in size and so should reuse old entries. That is,
if the process occupying entry 0 finishes, then the next time you go to launch a process,
it should go in that spot. When oss detects that a process has terminated, it can clear
out that entry and set its occupied to 0, indicating that it is now a free slot.

The task of oss is to launch a certain number of worker processes with particular
parameters. These numbers are determined by its own command line arguments.

Your solution will be invoked using the following command:

oss [-h] [-n proc] [-s simul] [-t timelimitForChildren] [-i intervalInMsToLaunchChildren]

While the first two parameters are similar to the previous project, the -t parameter is
different. It now stands for the upper bound of time that a child process will be launched
for. So for example, if it is called with -t 7, then when calling worker processes, it
should call them with a time interval randomly between 1 second and 7 seconds (with
nanoseconds also random).

The -i parameter is new. This specifies how often you should launch a child (based on the
system clock). For example, if -i 100 is passed, then your system would launch a child no
more than once every 100 milliseconds. This is set to allow user processes to slowly go
into the system, rather than flooding in initially.

When started, oss will initialize the system clock and then go into a loop and start doing
a fork() and then an exec() call to launch worker processes. However, it should only do
this up to simul number of times. So if called with a -s of 3, we would launch no more
than 3 initially. oss should make sure to update the process table with information as it
is launching user processes.

This seems close to what we did before, however, will not be doing wait() calls as before.
Instead, oss() will be going into a loop, incrementing the clock and then constantly
checking to see if a child has terminated. Rough pseudocode for this loop is below:

while (stillChildrenToLaunch) {
    incrementClock();
    // Every half a second of simulated clock time, output the process table to the screen
    checkIfChildHasTerminated();
    if (childHasTerminated) {
        updatePCBOfTerminatedChild;
    }
    possiblyLaunchNewChild(obeying process limits and time bound limits);
}

The check to see if a child has terminated should be done with a nonblocking wait() call.
This can be done with code along the lines of:

    int pid = waitpid(-1, &status, WNOHANG);

waitpid will return 0 if no child processes have terminated and will return the pid of the
child if one has terminated.

The output of oss should consist of, every half a second in our simulated system,
outputting the entire process table in a nice format. For example:

OSS PID:6576 SysClockS: 7 SysClockNano: 500000
Process Table:
Entry  Occupied  PID   StartS  StartN
0      1         6577  5       500000
1      0         0     0       0
2      0         0     0       0
...
19     0         0     0       0

Incrementing the clock:
Each iteration in oss you need to increment the clock. So how much should you increment
it? You should attempt to very loosely have your internal clock be similar to the real
clock. This does not have to be precise and does not need to be checked, just use it as a
crude guideline. So if you notice that your internal clock is much slower than real time,
increase your increment. If it is moving much faster, decrease your increment. Keep in
mind that this will change based on server load possibly, so do not worry about if it is
off sometimes and on other times.

Clock race conditions:
We are not doing explicit guarding against race conditions. As your child processes are
examining the clock, it is possible that the values they see will be slightly off
sometimes. This should not be a problem, as the only result would be a clock value that
was incorrect very briefly. This could cause the child process to possibly end early, but
I will be able to see that as long as your child processes are outputting the clock.

Signal Handling:
In addition, I expect your program to terminate after no more than 60 REAL LIFE seconds.
This can be done using a timeout signal, at which point it should kill all currently
running child processes and terminate. It should also catch the ctrl-c signal, free up
shared memory and then terminate all children.

This can be implemented by having your code send a termination signal after 60 seconds.

API for Shared Memory Operations(shared.c):
        create_shared_memory(key_t key, size_t size) - Creates and returns a shared memory
        segment with the given key and size. Exits on failure

        attach_shared_memory_rw(int shmid) - Attaches the shared memory segment (specified
by shmid) with read/write access. Returns a pointer to the attached memory. Mutex
        protection is applied for thread-safety. Exits on failure

        attach_shared_memory_ro(int shmid) - Attaches the shared memory segment (specified
by shmid) with read-only access. Returns a pointer to the attached memory. Mutex
protection is applied for thread-safety. Exits on failure

        detach_shared_memory(void* addr) - Detaches the shared memory segment at the given
        address. Mutex protection is applied. Exits on failure

        init_shared_memory_system(void) - Initializes the shared memory system, including
        semaphores for synchronization

        cleanup_shared_memory_system(void) - Cleans up the shared memory system, including
        semaphore destruction

API for Clock Operations(clock.c):
`increment_clock(struct SysClock* sys_clock)`
- Simulates advancing the system clock by a predefined `TICK_INTERVAL`.
- Handles overflow of nanoseconds into seconds.

`get_elapsed_time_ns(clock_t start_time)`
- Calculates and returns the elapsed time in nanoseconds since the provided `start_time`.

`get_system_clock()`
- Returns the current system time in a `struct SysClock`.

`set_system_clock(struct SysClock* sys_clock)`
- Sets the system clock to the specified `struct SysClock`.

*/
// oss.c

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "clock.h"
#include "shared.h"

int main( int argc, char *argv[] ) {
  (void)argc;
  (void)argv;

  // Initialize shared memory and semaphore system
  init_shared_memory_system();

  // Create and attach to the shared memory segment for SysClock
  int shmid                  = create_shared_memory( SHM_KEY, sizeof( struct SysClock ) );
  struct SysClock *sys_clock = (struct SysClock *)attach_shared_memory_rw( shmid );

  // Initialize the clock
  initialize_clock( sys_clock );

  // Start the clock increment loop
  start_clock_loop( sys_clock );

  // Detach shared memory once done
  detach_shared_memory( sys_clock );

  // Clean up shared memory and semaphore system
  cleanup_shared_memory_system();

  return 0;
}
