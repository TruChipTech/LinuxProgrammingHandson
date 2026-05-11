# Lab 15 — Temporary Files
CC = gcc
CFLAGS = -Wall -Wextra -g

.PHONY: all clean

all: tempfile_demo

tempfile_demo: tempfile_demo.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f tempfile_demo
