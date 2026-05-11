# Lab 13 — Memory Management
CC = gcc
CFLAGS = -Wall -Wextra -g

.PHONY: all clean

all: memory_demo memory_leaks

memory_demo: memory_demo.c
	$(CC) $(CFLAGS) $< -o $@

memory_leaks: memory_leaks.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f memory_demo memory_leaks
