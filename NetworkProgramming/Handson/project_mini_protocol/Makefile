# Project — Mini Protocol
CC      = gcc
CFLAGS  = -Wall -Wextra -g -O0
TARGETS = mini_server mini_client

all: $(TARGETS)

mini_server: mini_server.c protocol.h
	$(CC) $(CFLAGS) -o $@ mini_server.c

mini_client: mini_client.c protocol.h
	$(CC) $(CFLAGS) -o $@ mini_client.c

clean:
	rm -f $(TARGETS)

.PHONY: all clean
