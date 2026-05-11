# Lab 04 — Raw Sockets
CC      = gcc
CFLAGS  = -Wall -Wextra -g -O0
TARGETS = raw_sniffer ping_demo ethernet_sniffer

all: $(TARGETS)

raw_sniffer: raw_sniffer.c
	$(CC) $(CFLAGS) -o $@ $<

ping_demo: ping_demo.c
	$(CC) $(CFLAGS) -o $@ $<

ethernet_sniffer: ethernet_sniffer.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean
