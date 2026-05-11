# Lab 09 — Inter-Process Communication
CC = gcc
CFLAGS = -Wall -Wextra -g

.PHONY: all clean

all: pipe_demo fifo_demo mqueue_demo shm_demo unix_socket_demo

pipe_demo: pipe_demo.c
	$(CC) $(CFLAGS) $< -o $@

fifo_demo: fifo_demo.c
	$(CC) $(CFLAGS) $< -o $@

mqueue_demo: mqueue_demo.c
	$(CC) $(CFLAGS) $< -o $@ -lrt

shm_demo: shm_demo.c
	$(CC) $(CFLAGS) $< -o $@ -lrt -lpthread

unix_socket_demo: unix_socket_demo.c
	$(CC) $(CFLAGS) $< -o $@ -lpthread

clean:
	rm -f pipe_demo fifo_demo mqueue_demo shm_demo unix_socket_demo
	rm -f /tmp/my_fifo
