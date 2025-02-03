# "Note you MUST have a makefile that compiles and creates these executables at the same time.
#  This will require the use of the 'all' command in your makefile to compile multiple executables from the same makefile."
CC = gcc
CFLAGS = -Wall -g

all: p1/oss p1/user

p1/oss: p1/oss.c common/common.c
	$(CC) $(CFLAGS) -o p1/oss p1/oss.c common/common.c

p1/user: p1/user.c common/common.c
	$(CC) $(CFLAGS) -o p1/user p1/user.c common/common.c

clean:
	rm -f p1/oss p1/user p1/*.o common/*.o
