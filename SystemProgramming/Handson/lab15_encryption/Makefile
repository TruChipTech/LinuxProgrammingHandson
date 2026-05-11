# Lab 15 — Encryption Techniques
# Makefile

CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lssl -lcrypto

.PHONY: all clean

all: crypto_demo

crypto_demo: crypto_demo.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f crypto_demo *.enc *.pem *.bin *.sha256
