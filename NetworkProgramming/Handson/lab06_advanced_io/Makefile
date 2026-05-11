# Lab 06 — Advanced I/O
CC      = gcc
CFLAGS  = -Wall -Wextra -g -O0
TARGETS = select_server epoll_server

all: $(TARGETS)

select_server: select_server.c
	$(CC) $(CFLAGS) -o $@ $<

epoll_server: epoll_server.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean
