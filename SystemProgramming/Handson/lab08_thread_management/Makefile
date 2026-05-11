# Lab 08 — Thread Management
CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lpthread

.PHONY: all clean

all: thread_lab producer_consumer thread_pool

thread_lab: thread_lab.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

producer_consumer: producer_consumer.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

thread_pool: thread_pool.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f thread_lab producer_consumer thread_pool
