# Lab 06 — File Operations
CC = gcc
CFLAGS = -Wall -Wextra -g

.PHONY: all clean

all: file_ops file_benchmark mmap_demo file_watcher

file_ops: file_ops.c
	$(CC) $(CFLAGS) $< -o $@

file_benchmark: file_benchmark.c
	$(CC) $(CFLAGS) -O2 $< -o $@

mmap_demo: mmap_demo.c
	$(CC) $(CFLAGS) $< -o $@

file_watcher: file_watcher.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f file_ops file_benchmark mmap_demo file_watcher
