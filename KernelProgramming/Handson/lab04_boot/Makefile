# Lab 04 — Boot Sequence, Rootfs & Initramfs
CC = gcc
CFLAGS = -Wall -Wextra -g

.PHONY: all clean rootfs initramfs

all: custom_init

# Build as static binary (for use as init in minimal rootfs)
custom_init: custom_init.c
	$(CC) $(CFLAGS) -static $< -o $@

rootfs:
	bash build_minimal_rootfs.sh

initramfs:
	bash build_initramfs.sh

clean:
	rm -f custom_init
