// shared.h

#ifndef SHARED_H
#define SHARED_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/types.h>

#define SHM_KEY 12345

struct SysClock {
  int sec;
  int nano;
};

int create_shared_memory( key_t key, size_t size );
void *attach_shared_memory_rw( int shmid );
const void *attach_shared_memory_ro( int shmid );
void detach_shared_memory( void *addr );

void init_shared_memory_system( void );
void cleanup_shared_memory_system( void );

#endif  // SHARED_H
