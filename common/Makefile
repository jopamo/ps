# common/Makefile

CC      = gcc
CFLAGS  = -Wall -g
SOURCES = common.c
OBJECTS = $(SOURCES:.c=.o)
TARGET  = libcommon.a

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Archiving library $(TARGET)"
	ar rcs $(TARGET) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET)
