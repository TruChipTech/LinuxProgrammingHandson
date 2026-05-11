# Lab 03 — UDP
CC      = gcc
CFLAGS  = -Wall -Wextra -g -O0
TARGETS = udp_server udp_client udp_broadcast

all: $(TARGETS)

udp_server: udp_server.c
	$(CC) $(CFLAGS) -o $@ $<

udp_client: udp_client.c
	$(CC) $(CFLAGS) -o $@ $<

udp_broadcast: udp_broadcast.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean
