# Lab 18 — FUSE FAT Filesystem
CC = gcc
CFLAGS = -Wall -Wextra -g
FUSE_CFLAGS = $(shell pkg-config --cflags fuse3 2>/dev/null)
FUSE_LIBS = $(shell pkg-config --libs fuse3 2>/dev/null)

.PHONY: all clean test-image

all: fat_parser fuse_fat

fat_parser: fat_parser.c
	$(CC) $(CFLAGS) $< -o $@

fuse_fat: fuse_fat.c
	$(CC) $(CFLAGS) $(FUSE_CFLAGS) $< -o $@ $(FUSE_LIBS)

# Create a test FAT16 image for testing
test-image:
	dd if=/dev/zero of=test.img bs=1M count=16
	mkfs.vfat -F 16 -n TESTDISK test.img
	mkdir -p /tmp/fat_mount
	sudo mount -o loop test.img /tmp/fat_mount
	echo "Hello from FAT16!" | sudo tee /tmp/fat_mount/HELLO.TXT
	echo "Test file content" | sudo tee /tmp/fat_mount/README.TXT
	sudo mkdir /tmp/fat_mount/SUBDIR
	echo "Nested file" | sudo tee /tmp/fat_mount/SUBDIR/NESTED.TXT
	sudo umount /tmp/fat_mount
	rmdir /tmp/fat_mount
	@echo "Created test.img (16MB FAT16)"

clean:
	rm -f fat_parser fuse_fat test.img
