#ifndef SHARED_H
#define SHARED_H

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

#define SHM_KEY 1234567

struct SysClock {
  int sec;   // seconds part of the system clock
  int nano;  // nanoseconds part of the system clock
};

#define SHM_DEBUG 1
#if SHM_DEBUG
#define debug_log( message ) ( printf( "[SHARED DEBUG] %s\n", message ) )
#else
#define debug_log( message ) (void)message  // No-op when debugging is disabled
#endif

// Shared memory and semaphore functions
int create_shared_memory( key_t key, size_t size );
struct SysClock *attach_shared_memory_rw( int shmid );
const struct SysClock *attach_shared_memory_ro( int shmid );
void detach_shared_memory( void *addr );
void init_shared_memory_system( void );
void cleanup_shared_memory_system( void );
void cleanup_shared_memory( int shmid );
void handle_error( const char *msg );

// Signal handler setup
void setup_signal_handlers( void );

// Cleanup and exit function, invoked by signal handlers
void cleanup_and_exit( int signum );

#endif  // SHARED_H
