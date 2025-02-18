// shared.c

#include "shared.h"
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <unistd.h>

#define SHM_DEBUG 1

#if SHM_DEBUG
static void debug_log(const char *message) { printf("[SHARED DEBUG] %s\n", message); }
#else
static void debug_log(const char *message)
{
  (void) message; // No-op when debugging is disabled
}
#endif

static sem_t *shm_semaphore = NULL;

static void initialize_semaphore(void)
{
  shm_semaphore = sem_open("/shm_semaphore", O_CREAT, 0666, 1);
  if (shm_semaphore == SEM_FAILED) {
    perror("Failed to create semaphore");
    exit(1);
  }
}

static void destroy_semaphore(void)
{
  if (shm_semaphore != NULL) {
    sem_close(shm_semaphore);
    sem_unlink("/shm_semaphore");
  }
}

int create_shared_memory(key_t key, size_t size)
{
  int shmid = shmget(key, size, IPC_CREAT | 0666);
  if (shmid == -1) {
    perror("Failed to create or access shared memory");
    exit(1);
  }

#if SHM_DEBUG
  char log_msg[256];
  snprintf(log_msg,
           sizeof(log_msg),
           "Shared memory segment created/accessed. SHMID: %d, Size: %zu bytes",
           shmid,
           size);
  debug_log(log_msg);
#endif

  return shmid;
}

void *attach_shared_memory_rw(int shmid)
{
  sem_wait(shm_semaphore);

  void *addr = shmat(shmid, NULL, 0);
  if (addr == (void *) -1) {
    perror("Failed to attach shared memory (read/write)");
    exit(1);
  }

#if SHM_DEBUG
  char log_msg[256];
  snprintf(log_msg,
           sizeof(log_msg),
           "Attached shared memory (read/write). SHMID: %d, Address: %p",
           shmid,
           addr);
  debug_log(log_msg);
#endif

  sem_post(shm_semaphore);
  return addr;
}

const void *attach_shared_memory_ro(int shmid)
{
  sem_wait(shm_semaphore);

  void *addr = shmat(shmid, NULL, SHM_RDONLY);
  if (addr == (void *) -1) {
    perror("Failed to attach shared memory (read-only)");
    exit(1);
  }

#if SHM_DEBUG
  char log_msg[256];
  snprintf(log_msg,
           sizeof(log_msg),
           "Attached shared memory (read-only). SHMID: %d, Address: %p",
           shmid,
           addr);
  debug_log(log_msg);
#endif

  sem_post(shm_semaphore);
  return addr;
}

void detach_shared_memory(void *addr)
{
  sem_wait(shm_semaphore);

  if (shmdt(addr) == -1) {
    perror("Failed to detach shared memory");
    exit(1);
  }

#if SHM_DEBUG
  char log_msg[256];
  snprintf(log_msg, sizeof(log_msg), "Detached shared memory. Address: %p", addr);
  debug_log(log_msg);
#endif

  sem_post(shm_semaphore);
}

void init_shared_memory_system(void) { initialize_semaphore(); }

void cleanup_shared_memory_system(void) { destroy_semaphore(); }
