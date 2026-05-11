# Lab 16 — Timers
CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lrt -lpthread

.PHONY: all clean

all: timer_demo

timer_demo: timer_demo.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f timer_demo
