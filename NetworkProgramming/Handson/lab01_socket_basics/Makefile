# Lab 01 — Socket Basics
# Builds: socket_info, socket_options, packet_headers, dns_lookup, dns_client, ntp_client

CC      = gcc
CFLAGS  = -Wall -Wextra -g -O0
TARGETS = socket_info socket_options packet_headers dns_lookup dns_client ntp_client

all: $(TARGETS)

socket_info: socket_info.c
	$(CC) $(CFLAGS) -o $@ $<

socket_options: socket_options.c
	$(CC) $(CFLAGS) -o $@ $<

packet_headers: packet_headers.c
	$(CC) $(CFLAGS) -o $@ $<

dns_lookup: dns_lookup.c
	$(CC) $(CFLAGS) -o $@ $<

dns_client: dns_client.c
	$(CC) $(CFLAGS) -o $@ $<

ntp_client: ntp_client.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean
