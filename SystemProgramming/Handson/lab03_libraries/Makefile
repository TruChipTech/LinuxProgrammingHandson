# Lab 03 — Static & Dynamic Libraries
CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lm

.PHONY: all clean static shared test_static test_dynamic dlopen_test

all: static shared

# === Static Library ===
static: mathlib_basic.o mathlib_advanced.o
	ar rcs libmathutil.a mathlib_basic.o mathlib_advanced.o
	@echo "Built static library: libmathutil.a"
	ar t libmathutil.a

mathlib_basic.o: mathlib_basic.c mathlib.h
	$(CC) -c $(CFLAGS) $< -o $@

mathlib_advanced.o: mathlib_advanced.c mathlib.h
	$(CC) -c $(CFLAGS) $< -o $@ $(LDFLAGS)

# === Shared Library ===
shared: mathlib_basic_pic.o mathlib_advanced_pic.o
	$(CC) -shared -Wl,-soname,libmathutil.so.1 \
		-o libmathutil.so.1.0.0 $^ $(LDFLAGS)
	ln -sf libmathutil.so.1.0.0 libmathutil.so.1
	ln -sf libmathutil.so.1 libmathutil.so
	@echo "Built shared library: libmathutil.so"

mathlib_basic_pic.o: mathlib_basic.c mathlib.h
	$(CC) -c $(CFLAGS) -fPIC $< -o $@

mathlib_advanced_pic.o: mathlib_advanced.c mathlib.h
	$(CC) -c $(CFLAGS) -fPIC $< -o $@

# === Test programs ===
test_static: static test_mathlib.c
	$(CC) test_mathlib.c -L. -lmathutil -o test_static $(LDFLAGS)
	@echo "--- Static test ---"
	./test_static

test_dynamic: shared test_mathlib.c
	$(CC) test_mathlib.c -L. -lmathutil -o test_dynamic $(LDFLAGS)
	@echo "--- Dynamic test ---"
	LD_LIBRARY_PATH=. ./test_dynamic

dlopen_test: shared dlopen_demo.c
	$(CC) dlopen_demo.c -o dlopen_demo -ldl
	@echo "--- dlopen test ---"
	LD_LIBRARY_PATH=. ./dlopen_demo

compare: test_static test_dynamic
	@echo ""
	@echo "=== Size Comparison ==="
	@echo "Static binary:  $$(stat -c %s test_static) bytes"
	@echo "Dynamic binary: $$(stat -c %s test_dynamic) bytes"
	@echo "Shared library: $$(stat -c %s libmathutil.so.1.0.0) bytes"

clean:
	rm -f *.o *.a *.so* test_static test_dynamic dlopen_demo
