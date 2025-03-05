#ifndef SHARED_H
#define SHARED_H

#include <stddef.h>     // for size_t
#include <sys/types.h>  // for key_t, pid_t, etc.

#define SHM_KEY 0x1234

// Initialize the shared memory system (creates/opens a named semaphore)
void init_shared_memory_system( void );

// Cleanup the shared memory system (closes/unlinks the named semaphore)
void cleanup_shared_memory_system( void );

// Create a shared memory segment (returns shmid). Exits on error.
int create_shared_memory( key_t key, size_t size );

// Attach shared memory in read/write mode
void *attach_shared_memory_rw( int shmid );

// Attach shared memory in read-only mode (const pointer returned)
const void *attach_shared_memory_ro( int shmid );

// Detach from a shared memory segment
void detach_shared_memory( void *addr );

// Remove a shared memory segment from the system
void cleanup_shared_memory( int shmid );

// Setup signal handlers (e.g., SIGINT)
void setup_signal_handlers( void );

// General error handler
void handle_error( const char *msg );

#endif /* SHARED_H */
