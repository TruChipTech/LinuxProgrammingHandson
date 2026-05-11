# Lab 01 — Introduction to Linux Kernel
CC = gcc
CFLAGS = -Wall -Wextra -g

.PHONY: all clean

all: kernel_user_boundary

kernel_user_boundary: kernel_user_boundary.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f kernel_user_boundary
