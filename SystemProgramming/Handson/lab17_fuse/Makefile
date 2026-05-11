# Lab 17 — FUSE Filesystem
CC = gcc
CFLAGS = -Wall -Wextra -g $(shell pkg-config --cflags fuse3 2>/dev/null)
LDFLAGS = $(shell pkg-config --libs fuse3 2>/dev/null)

.PHONY: all clean

all: memfs

memfs: memfs.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f memfs
