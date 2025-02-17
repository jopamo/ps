# p1/Makefile

SHELL = /bin/bash

ifneq ($(shell which clang 2>/dev/null),)
  CC = clang
  CFLAGS = -Wall -Wextra -Werror -Wshadow -Wconversion -pedantic -O1 -g \
           -fsanitize=address,undefined -fno-omit-frame-pointer -I../tests
else
  CC = gcc
  CFLAGS = -Wall -g -I../tests
endif

TESTSDIR  = ../tests

OSS_SRC   = oss.c
USER_SRC  = user.c
OSS_EXE   = ../oss
USER_EXE  = ../user

OSS_OBJ   = oss.o
USER_OBJ  = user.o

TEST_SRC  = $(TESTSDIR)/test_p2.c
UNITY_SRC = $(TESTSDIR)/unity.c
TEST_OBJ  = test_p1.o
TEST_EXE  = test_p1

all: $(OSS_EXE) $(USER_EXE) optional_test

$(OSS_OBJ): $(OSS_SRC)
	$(CC) $(CFLAGS) -DTESTING -c $(OSS_SRC) -o $@

$(USER_OBJ): $(USER_SRC)
	$(CC) $(CFLAGS) -DTESTING -c $(USER_SRC) -o $@

$(OSS_EXE): $(OSS_SRC)
	$(CC) $(CFLAGS) -o $(OSS_EXE) $(OSS_SRC)

$(USER_EXE): $(USER_SRC)
	$(CC) $(CFLAGS) -o $(USER_EXE) $(USER_SRC)

optional_test:
ifneq ("$(wildcard $(TEST_SRC))$(wildcard $(UNITY_SRC))","")
	$(MAKE) $(TEST_EXE)
	@echo "Running $(TEST_EXE):"
	@./$(TEST_EXE)
else
	@echo "No test files found; skipping $(TEST_EXE)."
endif

$(TEST_EXE): $(TEST_OBJ) $(OSS_OBJ) $(USER_OBJ) $(UNITY_SRC)
	$(CC) $(CFLAGS) -o $(TEST_EXE) $(TEST_OBJ) $(UNITY_SRC) $(OSS_OBJ) $(USER_OBJ)

$(TEST_OBJ): $(TEST_SRC)
	$(CC) $(CFLAGS) -DTESTING -c $(TEST_SRC) -o $(TEST_OBJ)

clean:
	rm -f $(OSS_EXE) $(USER_EXE) $(TEST_EXE) *.o
