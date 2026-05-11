CC = gcc
CFLAGS = -g -O0 -Wall -Wextra -fno-omit-frame-pointer
ASAN_FLAGS = $(CFLAGS) -fsanitize=address,leak
TSAN_FLAGS = $(CFLAGS) -fsanitize=thread

.PHONY: all asan tsan clean

# Default: plain debug build
all: buggy_server buggy_client

buggy_server: buggy_server.c
	$(CC) $(CFLAGS) -o $@ $< -lpthread

buggy_client: buggy_client.c
	$(CC) $(CFLAGS) -o $@ $<

# ASan builds
asan: buggy_server_asan buggy_client_asan

buggy_server_asan: buggy_server.c
	$(CC) $(ASAN_FLAGS) -o $@ $< -lpthread

buggy_client_asan: buggy_client.c
	$(CC) $(ASAN_FLAGS) -o $@ $<

# TSan build (server only — has threading)
tsan: buggy_server_tsan

buggy_server_tsan: buggy_server.c
	$(CC) $(TSAN_FLAGS) -o $@ $< -lpthread

clean:
	rm -f buggy_server buggy_client \
	      buggy_server_asan buggy_client_asan \
	      buggy_server_tsan \
	      core* *.o
