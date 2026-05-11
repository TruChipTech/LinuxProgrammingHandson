# Lab 08 — Kernel Network Programming
#
# User-space tools:
CC      = gcc
CFLAGS  = -Wall -Wextra -g -O0

# Kernel module:
obj-m   += custom_protocol.o

KDIR    ?= /lib/modules/$(shell uname -r)/build

USER_TARGETS = send_custom_packet bpf_filter

all: modules user

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

user: $(USER_TARGETS)

send_custom_packet: send_custom_packet.c
	$(CC) $(CFLAGS) -o $@ $<

bpf_filter: bpf_filter.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f $(USER_TARGETS)

.PHONY: all modules user clean
