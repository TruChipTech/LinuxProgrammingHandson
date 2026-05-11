CC = gcc
CFLAGS = -g -O0 -Wall -Wextra -fno-omit-frame-pointer

TARGETS = buggy_app segfault_demo long_running thread_debug crash_scenarios

.PHONY: all clean

all: $(TARGETS)

buggy_app: buggy_app.c
	$(CC) $(CFLAGS) -o $@ $<

segfault_demo: segfault_demo.c
	$(CC) $(CFLAGS) -o $@ $<

long_running: long_running.c
	$(CC) $(CFLAGS) -o $@ $<

thread_debug: thread_debug.c
	$(CC) $(CFLAGS) -o $@ $< -lpthread

crash_scenarios: crash_scenarios.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS) core* *.o
