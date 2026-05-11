# Lab 19 — CPU Pinning
CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
CFLAGS_OPT = -Wall -O2 -pthread

.PHONY: all clean

all: cpu_pinning jitter_measure

cpu_pinning: cpu_pinning.c
	$(CC) $(CFLAGS) $< -o $@

jitter_measure: jitter_measure.c
	$(CC) $(CFLAGS_OPT) $< -o $@ -lrt

clean:
	rm -f cpu_pinning jitter_measure
