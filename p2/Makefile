SHELL = /bin/bash

ifneq ($(shell which clang 2>/dev/null),)
  CC = clang
  CFLAGS = -Wall -Wextra -Werror -Wshadow -Wconversion -pedantic -O1 -g \
           -fsanitize=address,undefined -fno-omit-frame-pointer -I./p2
else
  CC = gcc
  CFLAGS = -Wall -g -I./p2
endif

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

$(WORKER_OBJ): worker.c
	$(CC) $(CFLAGS) -c $^ -o $@

$(BOTH_OBJ): shared.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OSS_EXE): $(OSS_OBJ) $(BOTH_OBJ)
	$(CC) $(CFLAGS) -o $(OSS_EXE) $(OSS_OBJ) $(BOTH_OBJ)

$(WORKER_EXE): $(WORKER_OBJ) $(BOTH_OBJ)
	$(CC) $(CFLAGS) -o $(WORKER_EXE) $(WORKER_OBJ) $(BOTH_OBJ)

clean:
	rm -f $(OSS_EXE) $(WORKER_EXE) *.o

.PHONY: clean
