# Lab 04 — Error Handling & Assertions
CC = gcc
CFLAGS = -Wall -Wextra -g

.PHONY: all clean

all: error_handling_demo assert_demo assert_demo_release

error_handling_demo: error_handling_demo.c
	$(CC) $(CFLAGS) $< -o $@

assert_demo: assert_demo.c
	$(CC) $(CFLAGS) $< -o $@

# Build with assertions disabled
assert_demo_release: assert_demo.c
	$(CC) $(CFLAGS) -DNDEBUG $< -o $@

clean:
	rm -f error_handling_demo assert_demo assert_demo_release
