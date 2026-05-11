# Lab 02 — TCP Server/Client
CC      = gcc
CFLAGS  = -Wall -Wextra -g -O0
TARGETS = tcp_server tcp_client tcp_server_concurrent http_server

all: $(TARGETS)

tcp_server: tcp_server.c
	$(CC) $(CFLAGS) -o $@ $<

tcp_client: tcp_client.c
	$(CC) $(CFLAGS) -o $@ $<

tcp_server_concurrent: tcp_server_concurrent.c
	$(CC) $(CFLAGS) -o $@ $<

http_server: http_server.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean
