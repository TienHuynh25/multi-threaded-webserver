# Makefile for a1p3.c

CC = gcc
CFLAGS = -Wall
LDFLAGS =

SRC = a1p3.c
OBJ = $(SRC:.c=.o)
TARGET = a1p3

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean

