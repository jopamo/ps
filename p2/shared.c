#include "shared.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <unistd.h>

// Semaphore for controlling access to shared memory
static sem_t *shm_semaphore = NULL;

// Function to initialize the semaphore
static void initialize_semaphore( void ) {
  shm_semaphore = sem_open( "/shm_semaphore", O_CREAT | O_EXCL, 0666, 1 );
  if ( shm_semaphore == SEM_FAILED ) {
    if ( errno == EEXIST ) {
      // If semaphore exists, just open it
      shm_semaphore = sem_open( "/shm_semaphore", 0 );
      if ( shm_semaphore == SEM_FAILED ) {
        perror( "Failed to open existing semaphore" );
        exit( 1 );
      }
    } else {
      perror( "Failed to create or open semaphore" );
      exit( 1 );  // Exit if semaphore creation fails
    }
  }
  debug_log( "Semaphore initialized." );
}

// Destroy the semaphore
static void destroy_semaphore( void ) {
  if ( shm_semaphore != NULL ) {
    if ( sem_close( shm_semaphore ) == -1 ) {
      perror( "Failed to close semaphore" );
      exit( 1 );
    }
    if ( sem_unlink( "/shm_semaphore" ) == -1 ) {
      perror( "Failed to unlink semaphore" );
      exit( 1 );
    }
    debug_log( "Semaphore destroyed." );
  }
}

// Create shared memory segment
int create_shared_memory( key_t key, size_t size ) {
  int shmid = shmget( key, size, IPC_CREAT | 0666 );
  if ( shmid == -1 ) {
    perror( "Failed to create or access shared memory" );
    exit( 1 );  // Exit if shared memory creation fails
  }

  char log_msg[256];
  snprintf( log_msg, sizeof( log_msg ), "Shared memory created/accessed. SHMID: %d, Size: %zu bytes", shmid, size );
  debug_log( log_msg );

  return shmid;
}

// Attach to shared memory with read/write access
struct SysClock *attach_shared_memory_rw( int shmid ) {
  if ( sem_wait( shm_semaphore ) == -1 ) {
    perror( "Failed to acquire semaphore" );
    exit( 1 );
  }

  struct SysClock *addr = (struct SysClock *)shmat( shmid, NULL, 0 );
  if ( addr == (struct SysClock *)-1 ) {
    perror( "Failed to attach shared memory (read/write)" );
    sem_post( shm_semaphore );
    exit( 1 );
  }

  char log_msg[256];
  snprintf( log_msg, sizeof( log_msg ), "Attached shared memory (read/write). SHMID: %d, Address: %p", shmid,
            (void *)addr );
  debug_log( log_msg );

  if ( sem_post( shm_semaphore ) == -1 ) {
    perror( "Failed to release semaphore" );
    exit( 1 );
  }

  return addr;  // Return pointer to SysClock
}

// Attach to shared memory with read-only access (thread-safe using semaphore)
const struct SysClock *attach_shared_memory_ro( int shmid ) {
  if ( sem_wait( shm_semaphore ) == -1 ) {
    perror( "Failed to acquire semaphore" );
    exit( 1 );
  }

  const struct SysClock *addr = (const struct SysClock *)shmat( shmid, NULL, SHM_RDONLY );
  if ( addr == (const struct SysClock *)-1 ) {
    perror( "Failed to attach shared memory (read-only)" );
    sem_post( shm_semaphore );
    exit( 1 );
  }

  char log_msg[256];
  snprintf( log_msg, sizeof( log_msg ), "Attached shared memory (read-only). SHMID: %d, Address: %p", shmid,
            (void *)addr );
  debug_log( log_msg );

  if ( sem_post( shm_semaphore ) == -1 ) {
    perror( "Failed to release semaphore" );
    exit( 1 );
  }

  return addr;  // Return pointer to SysClock (read-only)
}

// Detach shared memory safely (thread-safe using semaphore)
void detach_shared_memory( void *addr ) {
  if ( sem_wait( shm_semaphore ) == -1 ) {
    perror( "Failed to acquire semaphore" );
    exit( 1 );
  }

  if ( shmdt( addr ) == -1 ) {
    perror( "Failed to detach shared memory" );
    sem_post( shm_semaphore );
    exit( 1 );
  }

  char log_msg[256];
  snprintf( log_msg, sizeof( log_msg ), "Detached shared memory. Address: %p", addr );
  debug_log( log_msg );

  if ( sem_post( shm_semaphore ) == -1 ) {
    perror( "Failed to release semaphore" );
    exit( 1 );
  }
}

// Initialize the shared memory system
void init_shared_memory_system( void ) {
  debug_log( "Initializing shared memory system." );
  initialize_semaphore();
}

// Cleanup the shared memory system
void cleanup_shared_memory_system( void ) {
  debug_log( "Cleaning up shared memory system." );
  destroy_semaphore();
}

// Cleanup shared memory segment by removing it
void cleanup_shared_memory( int shmid ) {
  if ( shmctl( shmid, IPC_RMID, NULL ) == -1 ) {
    perror( "Failed to remove shared memory segment" );
    exit( 1 );  // Exit if shared memory cleanup fails
  }
  debug_log( "Shared memory segment removed." );
}

// Cleanup and exit function (corrected signature for signal handler)
void cleanup_and_exit( int signum ) {
  (void)signum;  // Ignore the signal number as we just want to clean up
  debug_log( "Cleanup and exit signal received." );
  cleanup_shared_memory_system();
  exit( 0 );
}

// Setup signal handlers
void setup_signal_handlers( void ) {
  if ( signal( SIGALRM, cleanup_and_exit ) == SIG_ERR ) {
    perror( "Failed to set signal handler for SIGALRM" );
    exit( 1 );
  }
  if ( signal( SIGINT, cleanup_and_exit ) == SIG_ERR ) {
    perror( "Failed to set signal handler for SIGINT" );
    exit( 1 );
  }
}

// Error handling function
void handle_error( const char *msg ) {
  perror( msg );  // Print the error message
  cleanup_shared_memory_system();
  exit( EXIT_FAILURE );  // Exit with failure status
}
