# Lab 07 — Process Management
CC = gcc
CFLAGS = -Wall -Wextra -g

.PHONY: all clean

all: process_lab zombie_creator signal_handler

process_lab: process_lab.c
	$(CC) $(CFLAGS) $< -o $@

zombie_creator: zombie_creator.c
	$(CC) $(CFLAGS) $< -o $@

signal_handler: signal_handler.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f process_lab zombie_creator signal_handler
