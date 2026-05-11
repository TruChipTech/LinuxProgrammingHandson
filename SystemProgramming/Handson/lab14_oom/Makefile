# Lab 14 — OOM Killer
CC = gcc
CFLAGS = -Wall -Wextra -g

.PHONY: all clean

all: oom_demo

oom_demo: oom_demo.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f oom_demo
