/*
oss.c

**Adding Functionality to Our System**

**Purpose**:
This project builds upon the previous one by introducing additional functionality for future projects, specifically the
creation of a simulated system clock for managing process life cycles, rather than relying on system time.

**Task**:
There are two executables: `oss` (parent) and `worker` (child). The key difference from the previous project is that the
processes will run based on a custom clock, not system time. You must avoid using functions like `sleep` or `usleep`
that rely on system time.

- **Simulated System Clock**:
  `oss` maintains a simulated clock in shared memory, consisting of two integers: one for seconds and the other for
nanoseconds. This clock starts at zero and is accessible to children, though they won't modify it.

- **Process Table**:
  `oss` maintains a process table that tracks each child process using Process Control Blocks (PCBs). The table stores
the child's PID, the start time (from the simulated clock), and whether the entry is in use. The table has a fixed size
and reuses entries when a child process terminates.

  Example structure for PCB:
  ```c
  struct PCB {
    int occupied; // true or false
    pid_t pid;    // process id
    int startSeconds; // start time in seconds
    int startNano;    // start time in nanoseconds
  };
  struct PCB processTable[20];
  ```

- **Worker (Child Process)**:
  The worker process accepts two command-line arguments indicating the maximum time it should stay active in the system.
It attaches to the shared memory, checks the simulated clock, and calculates when it should terminate. The worker
constantly monitors the clock to decide when to exit, outputting periodic status updates.

  Example worker execution:
  ```
  ./worker 5 500000
  ```

  Example of worker output:
  ```
  WORKER PID:6577 PPID:6576 SysClockS: 5 SysClockNano: 500000000 TermTimeS: 11 TermTimeNano: 800000000 -- Just Starting
  WORKER PID:6577 PPID:6576 SysClockS: 6 SysClockNano: 900000000 TermTimeS: 11 TermTimeNano: 800000000 -- 1 second
passed
  ...
  WORKER PID:6577 PPID:6576 SysClockS: 11 SysClockNano: 800000000 TermTimeS: 11 TermTimeNano: 800000000 -- Terminating
  ```

- **oss (Parent Process)**:
  The `oss` executable manages launching multiple worker processes based on the command line arguments. It is
responsible for initializing the system clock, launching workers, and updating the process table. The `oss` process does
not use `wait()` calls but increments the clock and checks for terminated children periodically.

  Command to run `oss`:
  ```
  oss [-h] [-n proc] [-s simul] [-t timelimitForChildren] [-i intervalInMsToLaunchChildren]
  ```

  - `-n proc`: Number of child processes.
  - `-s simul`: Simulation parameter.
  - `-t timelimitForChildren`: The upper bound time for each worker (in seconds).
  - `-i intervalInMsToLaunchChildren`: Interval in milliseconds between child launches.

  The `oss` process runs in a loop, incrementing the clock and checking for terminated children using non-blocking
`waitpid()`. It will output the process table every half second, showing the current clock and the state of each child
process.

  Example of output:
  ```
  OSS PID:6576 SysClockS: 7 SysClockNano: 500000
  Process Table:
  Entry Occupied PID StartS StartN
  0 1 6577 5 500000
  ...
  ```

- **Clock Management**:
  Each time `oss` increments the clock, it attempts to keep it roughly in sync with the real-time clock. The increment
is adjusted dynamically based on the speed of the internal clock. Although race conditions may occur when child
processes examine the clock, it is not a significant issue for this project.

- **Signal Handling**:
  The program should terminate after 60 real-life seconds by sending a timeout signal. Upon termination, it must kill
all running children and clean up shared memory. The program should also handle `Ctrl-C` to properly clean up and
terminate.

  This can be implemented by sending a termination signal after 60 seconds and handling cleanup using signals such as
`SIGTERM`.

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
