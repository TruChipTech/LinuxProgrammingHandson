CC ?= gcc
CFLAGS := -Wall -Wextra -O2

.PHONY: all clean

all: v4l2_capture

v4l2_capture: v4l2_capture.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f v4l2_capture
