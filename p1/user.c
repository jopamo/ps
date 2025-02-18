/*
 The user takes in one command line argument. For example, if you were
 running it directly you would call it like:
   ./user 5

  As it is being called with the number 5, it would do 5 iterations over a loop.
  So what does it do in that loop? Each iteration it will output its PID, its parents PID,
  and what iteration of the loop it is in.

  After doing this output, it should do sleep(1), to sleep for one second, and then output:
    USER PID:6577 PPID:6576 Iteration:3 after sleeping
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int parseIterations( const char* arg ) {
  int val = atoi( arg );
  if ( val <= 0 ) {
    return -1;
  }
  return val;
}

#ifndef TESTING
int main( int argc, char* argv[] ) {
  if ( argc != 2 ) {
    fprintf( stderr, "Usage: %s <iterations>\n", argv[0] );
    return 1;
  }

  int iterations = parseIterations( argv[1] );
  if ( iterations < 0 ) {
    fprintf( stderr, "Error: iterations must be positive\n" );
    return 1;
  }

  pid_t pid  = getpid();
  pid_t ppid = getppid();

  for ( int i = 1; i <= iterations; i++ ) {
    printf( "USER PID:%d PPID:%d Iteration:%d before sleeping\n", pid, ppid, i );
    fflush( stdout );
    sleep( 1 );
    printf( "USER PID:%d PPID:%d Iteration:%d after sleeping\n", pid, ppid, i );
    fflush( stdout );
  }
  return 0;
}
#endif
