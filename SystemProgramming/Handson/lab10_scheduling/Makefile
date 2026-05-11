# Lab 10 — Process Scheduling
CC = gcc
CFLAGS = -Wall -Wextra -g

.PHONY: all clean

all: sched_demo

sched_demo: sched_demo.c
	$(CC) $(CFLAGS) $< -o $@ -lpthread

clean:
	rm -f sched_demo
