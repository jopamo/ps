SHELL = /bin/bash

ifneq ($(shell which clang 2>/dev/null),)
  CC = clang
  CFLAGS = -Wall -Wextra -Werror -Wshadow -Wconversion -pedantic -O1 -g -fno-omit-frame-pointer -I./p2
else
  CC = gcc
  CFLAGS = -Wall -g -I./p2
endif

# Link libraries for POSIX semaphores
# -pthread suffices on many modern systems; -lrt is needed on some older or specific environments.
LDLIBS = -pthread -lrt

OSS_SRC = oss.c clock.c
WORKER_SRC = worker.c
BOTH_SRC = shared.c

OSS_OBJ = oss.o clock.o
WORKER_OBJ = worker.o
BOTH_OBJ = shared.o

OSS_EXE = ../oss
WORKER_EXE = ../worker

all: $(OSS_EXE) $(WORKER_EXE)

oss.o: oss.c
	$(CC) $(CFLAGS) -c $< -o $@

clock.o: clock.c
	$(CC) $(CFLAGS) -c $< -o $@

worker.o: worker.c
	$(CC) $(CFLAGS) -c $< -o $@

shared.o: shared.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OSS_EXE): $(OSS_OBJ) $(BOTH_OBJ)
	$(CC) $(CFLAGS) -o $@ $(OSS_OBJ) $(BOTH_OBJ) $(LDLIBS)

$(WORKER_EXE): $(WORKER_OBJ) $(BOTH_OBJ)
	$(CC) $(CFLAGS) -o $@ $(WORKER_OBJ) $(BOTH_OBJ) $(LDLIBS)

clean:
	rm -f $(OSS_EXE) $(WORKER_EXE) *.o

.PHONY: clean
