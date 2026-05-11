CC = gcc
CFLAGS = -g -O2 -Wall

.PHONY: all clean

all: cpu_hog

cpu_hog: cpu_hog.c
	$(CC) $(CFLAGS) -o $@ $< -lm

clean:
	rm -f cpu_hog *.o perf.data perf.data.old
