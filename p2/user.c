#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/*
  The worker takes in two command line arguments, this time corresponding to the maximum
  time it should decide to stay around in the system.
*/
#ifndef TESTING
int main(int argc, char *argv[])
{
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <maxSeconds> <maxNanoseconds>\n", argv[0]);
    return 1;
  }

  int max_seconds = atoi(argv[1]);
  int max_nanoseconds = atoi(argv[2]);

  /*
    The worker will then attach to shared memory and examine our simulated system
    clock. It will then figure out what time it should terminate by adding up the
    system clock time and the time passed to it (in our simulated system clock,
    not actual time).
  */
  key_t key = ftok("oss", 65);
  int shm_id = shmget(key, sizeof(struct SimulatedClock), 0666 | IPC_CREAT);
  struct SimulatedClock *simulated_clock = (struct SimulatedClock *) shmat(shm_id, NULL, 0);

  /*
    If the system clock was showing 6 seconds and 100 nanoseconds and the worker
    was passed 5 and 500000 as above, the target time to terminate in the system
    would be 11 seconds and 500100 nanoseconds. The worker will then go into a loop,
    constantly checking the system clock to see if this time has passed.
  */
  int target_seconds = simulated_clock->seconds + max_seconds;
  int target_nanoseconds = simulated_clock->nanoseconds + max_nanoseconds;

  while (simulated_clock->seconds < target_seconds ||
         (simulated_clock->seconds == target_seconds && simulated_clock->nanoseconds < target_nanoseconds)) {
    /*
      The worker should then go into a loop, checking for its time to expire
      (IT SHOULD NOT DO A SLEEP). It should also do some periodic output.
    */
    if (simulated_clock->seconds != target_seconds || simulated_clock->nanoseconds != target_nanoseconds) {
      /*
        Every time it notices that the seconds have changed, it should output a message like:
        WORKER PID:6577 PPID:6576 SysClockS: 6 SysclockNano: 45000000 TermTimeS: 11 TermTimeNano: 500100
        --1 seconds have passed since starting
      */
      printf("WORKER PID:%d PPID:%d SysClockS:%d SysClockNano:%d TermTimeS:%d TermTimeNano:%d\n",
             getpid(),
             getppid(),
             simulated_clock->seconds,
             simulated_clock->nanoseconds,
             target_seconds,
             target_nanoseconds);
      fflush(stdout);
    }

    /*
      The worker will check the clock again until the time has passed.
    */
    usleep(100000); // Sleep for a very short time to avoid busy-waiting
  }

  /*
    Once its time has elapsed, it would send out one final message:
    WORKER PID:6577 PPID:6576 SysClockS: 11 SysClockNano: 700000 TermTimeS: 11 TermTimeNano: 500100
    --Terminating
  */
  printf("WORKER PID:%d PPID:%d SysClockS:%d SysClockNano:%d TermTimeS:%d TermTimeNano:%d\n",
         getpid(),
         getppid(),
         simulated_clock->seconds,
         simulated_clock->nanoseconds,
         target_seconds,
         target_nanoseconds);

  return 0;
}
#endif
