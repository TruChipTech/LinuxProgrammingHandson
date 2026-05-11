# Lab 06 — System Calls
CC = gcc
CFLAGS = -Wall -Wextra -g

.PHONY: all clean

all: syscall_trace raw_syscall

syscall_trace: syscall_trace.c
	$(CC) $(CFLAGS) $< -o $@

raw_syscall: raw_syscall.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f syscall_trace raw_syscall
