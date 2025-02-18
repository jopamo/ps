# SHELL and compiler detection
SHELL = /bin/bash

# Check for clang, otherwise fallback to gcc
ifneq ($(shell which clang 2>/dev/null),)
  CC = clang
  CFLAGS = -Wall -Wextra -Werror -Wshadow -Wconversion -pedantic -O1 -g \
           -fsanitize=address,undefined -fno-omit-frame-pointer -I./p2
else
  CC = gcc
  CFLAGS = -Wall -g -I./p2
endif

# Source files
OSS_SRC = oss.c clock.c   # OSS files: oss.c (main), clock.c (helpers)
WORKER_SRC = worker.c     # Worker files: worker.c
BOTH_SRC = shared.c       # Shared functionality

# Object files
OSS_OBJ = oss.o clock.o   # Object files for OSS (oss.o, clock.o)
WORKER_OBJ = worker.o     # Object files for Worker (worker.o)
BOTH_OBJ = shared.o       # Object file for shared.c (shared.o)

# Executables
OSS_EXE = ../oss
WORKER_EXE = ../worker

# Default target
all: $(OSS_EXE) $(WORKER_EXE)

# Compile OSS source files into object files
oss.o: oss.c
	$(CC) $(CFLAGS) -c $< -o $@

clock.o: clock.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile Worker source files into object files
$(WORKER_OBJ): worker.c
	$(CC) $(CFLAGS) -c $^ -o $@

# Compile shared source files (used by both)
$(BOTH_OBJ): shared.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link OSS object files and shared objects into final executable
$(OSS_EXE): $(OSS_OBJ) $(BOTH_OBJ)
	$(CC) $(CFLAGS) -o $(OSS_EXE) $(OSS_OBJ) $(BOTH_OBJ)

# Link Worker object files and shared objects into final executable
$(WORKER_EXE): $(WORKER_OBJ) $(BOTH_OBJ)
	$(CC) $(CFLAGS) -o $(WORKER_EXE) $(WORKER_OBJ) $(BOTH_OBJ)

# Clean up generated files (object files and executables)
clean:
	rm -f $(OSS_EXE) $(WORKER_EXE) *.o

# Phony target for cleaning
.PHONY: clean
