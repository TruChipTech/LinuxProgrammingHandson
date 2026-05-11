# Lab 12 — Users & Groups
CC = gcc
CFLAGS = -Wall -Wextra -g

.PHONY: all clean

all: user_info priv_drop

user_info: user_info.c
	$(CC) $(CFLAGS) $< -o $@

priv_drop: priv_drop.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f user_info priv_drop
