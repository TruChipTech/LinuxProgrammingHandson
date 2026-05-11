# Lab 01 — GCC Compilation Stages & Optimization
# Makefile for building all exercises

CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lm

.PHONY: all clean pipeline macros mixed optimization pgo specific_opts

all: pipeline macros optimization specific_opts

# === Exercise 1.1: Pipeline Demo ===
pipeline: pipeline_demo.c
	@echo "=== Stage 1: Preprocessing ==="
	$(CC) -E $< -o pipeline_demo.i
	@echo "Preprocessed lines: $$(wc -l < pipeline_demo.i)"
	@echo ""
	@echo "=== Stage 2: Compilation (to assembly) ==="
	$(CC) -S pipeline_demo.i -o pipeline_demo.s
	@echo "Assembly lines: $$(wc -l < pipeline_demo.s)"
	@echo ""
	@echo "=== Stage 3: Assembly (to object) ==="
	$(CC) -c pipeline_demo.s -o pipeline_demo.o
	@file pipeline_demo.o
	@echo ""
	@echo "=== Stage 4: Linking ==="
	$(CC) pipeline_demo.o -o pipeline_demo $(LDFLAGS)
	@file pipeline_demo
	@echo ""
	@echo "=== Running ==="
	./pipeline_demo

# === Exercise 1.2: Macro Magic ===
macros: macro_magic.c
	$(CC) $(CFLAGS) -DPLATFORM_X86 $< -o macro_x86
	$(CC) $(CFLAGS) -DPLATFORM_ARM $< -o macro_arm
	@echo "=== x86 build ==="
	./macro_x86
	@echo ""
	@echo "=== ARM build ==="
	./macro_arm

# === Exercise 1.3: Mixed Language ===
mixed: main_caller.c asm_add.s
	nasm -f elf64 asm_add.s -o asm_add.o
	$(CC) -c main_caller.c -o main_caller.o
	$(CC) main_caller.o asm_add.o -o mixed_lang
	./mixed_lang

# === Exercise 2.1: Optimization Benchmark ===
optimization: optimization_benchmark.c
	@echo "Compiling at all optimization levels..."
	@for opt in O0 O1 O2 O3 Os Ofast; do \
		$(CC) -$${opt} $< -o bench_$${opt} $(LDFLAGS); \
		echo "bench_$${opt}: $$(stat -c %s bench_$${opt}) bytes"; \
	done
	@echo ""
	@echo "Run 'time ./bench_O0' etc. to compare performance"

# === Exercise 2.2: Specific Optimizations ===
specific_opts: specific_opts.c
	$(CC) -O0 $< -o spec_base
	$(CC) -O0 -fdce $< -o spec_dce
	$(CC) -O0 -funroll-loops $< -o spec_unroll
	$(CC) -O0 -finline-functions $< -o spec_inline
	@echo "Built spec_base, spec_dce, spec_unroll, spec_inline"
	@echo "Use objdump -d <binary> to compare"

# === Exercise 2.3: PGO ===
pgo: pgo_demo.c
	@echo "=== Step 1: Instrumented build ==="
	$(CC) -O2 -fprofile-generate $< -o pgo_instrument $(LDFLAGS)
	@echo ""
	@echo "=== Step 2: Training run ==="
	./pgo_instrument
	@echo ""
	@echo "=== Step 3: PGO-optimized build ==="
	$(CC) -O2 -fprofile-use -fprofile-correction $< -o pgo_optimized $(LDFLAGS)
	@echo ""
	@echo "=== Step 4: Plain O2 build for comparison ==="
	$(CC) -O2 $< -o pgo_plain $(LDFLAGS)
	@echo ""
	@echo "Now compare: time ./pgo_plain  vs  time ./pgo_optimized"

clean:
	rm -f *.i *.s *.o *.gcda *.gcno *.gcov
	rm -f pipeline_demo macro_x86 macro_arm mixed_lang
	rm -f bench_* spec_* pgo_instrument pgo_optimized pgo_plain
