# ./p1/Makefile
CC = gcc
CFLAGS = -Wall -g

all: oss user

oss: oss.c ../common/common.c
	$(CC) $(CFLAGS) -o oss oss.c ../common/common.c

user: user.c ../common/common.c
	$(CC) $(CFLAGS) -o user user.c ../common/common.c

clean:
	rm -f oss user *.o
