# Lab 05 — Unix Domain Sockets
CC      = gcc
CFLAGS  = -Wall -Wextra -g -O0
TARGETS = unix_server unix_client fd_passing

all: $(TARGETS)

unix_server: unix_server.c
	$(CC) $(CFLAGS) -o $@ $<

unix_client: unix_client.c
	$(CC) $(CFLAGS) -o $@ $<

fd_passing: fd_passing.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean
