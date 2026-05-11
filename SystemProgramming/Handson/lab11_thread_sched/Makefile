# Lab 11 — Thread Scheduling
CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lpthread

.PHONY: all clean

all: thread_sched

thread_sched: thread_sched.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f thread_sched
