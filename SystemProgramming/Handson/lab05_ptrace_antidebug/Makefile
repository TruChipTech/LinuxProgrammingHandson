# Lab 05 — Ptrace & Anti-Debugging
CC = gcc
CFLAGS = -Wall -Wextra -g

.PHONY: all clean

all: mini_strace memspy target_prog anti_debug fake_ptrace.so

mini_strace: mini_strace.c
	$(CC) $(CFLAGS) $< -o $@

memspy: ptrace_memspy.c
	$(CC) $(CFLAGS) $< -o $@

target_prog: target_program.c
	$(CC) $(CFLAGS) $< -o $@

anti_debug: anti_debug.c
	$(CC) $(CFLAGS) $< -o $@

fake_ptrace.so: fake_ptrace.c
	$(CC) -shared -fPIC $< -o $@

clean:
	rm -f mini_strace memspy target_prog anti_debug fake_ptrace.so
