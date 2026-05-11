# Lab 07 — Netfilter / iptables
CC      = gcc
CFLAGS  = -Wall -Wextra -g -O0
LDFLAGS = -lnetfilter_queue
TARGETS = nfqueue_demo

all: $(TARGETS)

nfqueue_demo: nfqueue_demo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean
