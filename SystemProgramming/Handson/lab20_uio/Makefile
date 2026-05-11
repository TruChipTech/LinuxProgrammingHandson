# Lab 20 — UIO Driver
CC = gcc
CFLAGS = -Wall -Wextra -g

.PHONY: all clean kmod sim_device

all: uio_user uio_sim_user

uio_user: uio_user.c
	$(CC) $(CFLAGS) $< -o $@

uio_sim_user: uio_sim_user.c
	$(CC) $(CFLAGS) $< -o $@

kmod:
	$(MAKE) -C kmod

sim_device:
	$(MAKE) -C sim_device

clean:
	rm -f uio_user uio_sim_user
	$(MAKE) -C kmod clean 2>/dev/null || true
	$(MAKE) -C sim_device clean 2>/dev/null || true
