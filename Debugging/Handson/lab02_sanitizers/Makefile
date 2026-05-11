CC = gcc
CFLAGS_BASE = -g -O0 -Wall -Wextra -fno-omit-frame-pointer

# ASan targets
ASAN_FLAGS = $(CFLAGS_BASE) -fsanitize=address
ASAN_LEAK_FLAGS = $(CFLAGS_BASE) -fsanitize=address,leak

# TSan targets
TSAN_FLAGS = $(CFLAGS_BASE) -fsanitize=thread

ASAN_TARGETS = heap_overflow use_after_free stack_overflow memory_leak
TSAN_TARGETS = data_race

# Also build unsafe versions for comparison
UNSAFE_TARGETS = heap_overflow_unsafe

.PHONY: all asan tsan unsafe clean

all: asan tsan unsafe

asan: $(ASAN_TARGETS)
tsan: $(TSAN_TARGETS)
unsafe: $(UNSAFE_TARGETS)

heap_overflow: heap_overflow.c
	$(CC) $(ASAN_FLAGS) -o $@ $<

heap_overflow_unsafe: heap_overflow.c
	$(CC) $(CFLAGS_BASE) -o $@ $<

use_after_free: use_after_free.c
	$(CC) $(ASAN_FLAGS) -o $@ $<

stack_overflow: stack_overflow.c
	$(CC) $(ASAN_FLAGS) -o $@ $<

memory_leak: memory_leak.c
	$(CC) $(ASAN_LEAK_FLAGS) -o $@ $<

data_race: data_race.c
	$(CC) $(TSAN_FLAGS) -o $@ $< -lpthread

clean:
	rm -f $(ASAN_TARGETS) $(TSAN_TARGETS) $(UNSAFE_TARGETS) core* *.o
