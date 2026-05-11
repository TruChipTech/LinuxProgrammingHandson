CC = gcc
CFLAGS = -g -O0 -Wall -Wextra

TARGETS = leak_demo invalid_access

.PHONY: all clean

all: $(TARGETS)

leak_demo: leak_demo.c
	$(CC) $(CFLAGS) -o $@ $<

invalid_access: invalid_access.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS) core* *.o
