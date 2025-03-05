#include "shared.h"
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>

static sem_t *shm_semaphore = NULL;
static const char *SEM_NAME = "/shm_semaphore";

static void signal_handler(int signum) {
    switch (signum) {
    case SIGINT:
    case SIGTERM:
    case SIGALRM:
        _exit(EXIT_SUCCESS);
    default:
        _exit(EXIT_FAILURE);
    }
}

void init_shared_memory_system(void) {
    shm_semaphore = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 1);
    if (shm_semaphore == SEM_FAILED) {
        if (errno == EEXIST) {
            shm_semaphore = sem_open(SEM_NAME, 0);
            if (shm_semaphore == SEM_FAILED) {
                perror("sem_open existing");
                exit(EXIT_FAILURE);
            }
        } else {
            perror("sem_open create");
            exit(EXIT_FAILURE);
        }
    }
}

void cleanup_shared_memory_system(void) {
    if (shm_semaphore) {
        sem_close(shm_semaphore);
        sem_unlink(SEM_NAME);
        shm_semaphore = NULL;
    }
}

int create_shared_memory(key_t key, size_t size) {
    int shmid = shmget(key, size, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    return shmid;
}

void *attach_shared_memory_rw(int shmid) {
    if (sem_wait(shm_semaphore) == -1) {
        perror("sem_wait attach RW");
        exit(EXIT_FAILURE);
    }
    void *addr = shmat(shmid, NULL, 0);
    if (addr == (void *)-1) {
        perror("shmat RW");
        sem_post(shm_semaphore);
        exit(EXIT_FAILURE);
    }
    if (sem_post(shm_semaphore) == -1) {
        perror("sem_post attach RW");
        exit(EXIT_FAILURE);
    }
    return addr;
}

const void *attach_shared_memory_ro(int shmid) {
    if (sem_wait(shm_semaphore) == -1) {
        perror("sem_wait attach RO");
        exit(EXIT_FAILURE);
    }
    const void *addr = shmat(shmid, NULL, SHM_RDONLY);
    if (addr == (void *)-1) {
        perror("shmat RO");
        sem_post(shm_semaphore);
        exit(EXIT_FAILURE);
    }
    if (sem_post(shm_semaphore) == -1) {
        perror("sem_post attach RO");
        exit(EXIT_FAILURE);
    }
    return addr;
}

void detach_shared_memory(void *addr) {
    if (sem_wait(shm_semaphore) == -1) {
        perror("sem_wait detach");
        exit(EXIT_FAILURE);
    }
    if (shmdt(addr) == -1) {
        perror("shmdt");
        sem_post(shm_semaphore);
        exit(EXIT_FAILURE);
    }
    if (sem_post(shm_semaphore) == -1) {
        perror("sem_post detach");
        exit(EXIT_FAILURE);
    }
}

void cleanup_shared_memory(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID");
    }
}

void setup_signal_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction SIGTERM");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction SIGALRM");
        exit(EXIT_FAILURE);
    }
}

void handle_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}
