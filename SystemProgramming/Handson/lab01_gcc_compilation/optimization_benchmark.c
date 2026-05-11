/*
 * optimization_benchmark.c — GCC Optimization Level Benchmark
 *
 * Compile at different levels to compare performance:
 *   gcc -O0 optimization_benchmark.c -o bench_O0 -lm
 *   gcc -O1 optimization_benchmark.c -o bench_O1 -lm
 *   gcc -O2 optimization_benchmark.c -o bench_O2 -lm
 *   gcc -O3 optimization_benchmark.c -o bench_O3 -lm
 *   gcc -Os optimization_benchmark.c -o bench_Os -lm
 *   gcc -Ofast optimization_benchmark.c -o bench_Ofast -lm
 *
 * Run with: time ./bench_O0
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define ITERATIONS      100000000
#define ARRAY_SIZE      10000
#define SORT_ITERATIONS 500

/* ============================================================
 * Benchmark 1: Arithmetic-heavy loop
 * Tests: loop unrolling, strength reduction, constant propagation
 * ============================================================ */
double benchmark_arithmetic(void) {
    double result = 0.0;
    for (long i = 1; i <= ITERATIONS; i++) {
        result += 1.0 / (double)(i * i);
    }
    return result;
}

/* ============================================================
 * Benchmark 2: Function call overhead
 * Tests: function inlining
 * ============================================================ */
static int tiny_add(int a, int b) {
    return a + b;
}

long benchmark_function_calls(void) {
    long result = 0;
    for (long i = 0; i < ITERATIONS; i++) {
        result = tiny_add(result, 1);
    }
    return result;
}

/* ============================================================
 * Benchmark 3: Array operations
 * Tests: vectorization (SIMD), loop optimization
 * ============================================================ */
void benchmark_array_ops(int *a, int *b, int *c, int n) {
    /* This loop is a prime candidate for auto-vectorization */
    for (int i = 0; i < n; i++) {
        c[i] = a[i] * b[i] + a[i] - b[i];
    }
}

/* ============================================================
 * Benchmark 4: Dead code and unreachable paths
 * Tests: dead code elimination
 * ============================================================ */
int benchmark_dead_code(int input) {
    int result = input;
    
    /* This branch is never taken for positive inputs */
    if (input < 0 && input > 100) {
        /* Dead code — impossible condition */
        result = result * 1000;
        result = result / 3;
        result = result + 999;
        for (int i = 0; i < 1000; i++) {
            result += i;
        }
    }
    
    /* Redundant computations */
    int x = input * 2;
    int y = input * 2;  /* Compiler should recognize x == y */
    result = x + y;
    
    return result;
}

/* ============================================================
 * Benchmark 5: Bubble Sort (branch-heavy)
 * Tests: branch prediction optimization
 * ============================================================ */
void bubble_sort(int *arr, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }
}

/* ============================================================
 * MAIN — Run all benchmarks
 * ============================================================ */
int main(void) {
    printf("=== GCC Optimization Benchmark ===\n\n");
    
    /* Benchmark 1 */
    printf("[1/5] Arithmetic benchmark (%.0e iterations)...\n",
           (double)ITERATIONS);
    double arith_result = benchmark_arithmetic();
    printf("  Result: %.10f (π²/6 ≈ 1.6449340668)\n\n", arith_result);
    
    /* Benchmark 2 */
    printf("[2/5] Function call benchmark...\n");
    long call_result = benchmark_function_calls();
    printf("  Result: %ld\n\n", call_result);
    
    /* Benchmark 3 */
    printf("[3/5] Array operations benchmark...\n");
    int *a = malloc(ARRAY_SIZE * sizeof(int));
    int *b = malloc(ARRAY_SIZE * sizeof(int));
    int *c = malloc(ARRAY_SIZE * sizeof(int));
    
    for (int i = 0; i < ARRAY_SIZE; i++) {
        a[i] = i;
        b[i] = ARRAY_SIZE - i;
    }
    
    for (int iter = 0; iter < 100000; iter++) {
        benchmark_array_ops(a, b, c, ARRAY_SIZE);
    }
    printf("  Result: c[0]=%d, c[%d]=%d\n\n", c[0], ARRAY_SIZE/2,
           c[ARRAY_SIZE/2]);
    
    /* Benchmark 4 */
    printf("[4/5] Dead code benchmark...\n");
    int dead_result = 0;
    for (int i = 0; i < ITERATIONS/10; i++) {
        dead_result += benchmark_dead_code(i);
    }
    printf("  Result: %d\n\n", dead_result);
    
    /* Benchmark 5 */
    printf("[5/5] Sort benchmark (%d iterations)...\n", SORT_ITERATIONS);
    int *sort_arr = malloc(ARRAY_SIZE * sizeof(int));
    for (int iter = 0; iter < SORT_ITERATIONS; iter++) {
        for (int i = 0; i < ARRAY_SIZE; i++) {
            sort_arr[i] = ARRAY_SIZE - i;  /* Worst case: reverse sorted */
        }
        bubble_sort(sort_arr, ARRAY_SIZE / 10);  /* Use subset for speed */
    }
    printf("  Sorted: arr[0]=%d, arr[1]=%d\n\n", sort_arr[0], sort_arr[1]);
    
    /* Cleanup */
    free(a);
    free(b);
    free(c);
    free(sort_arr);
    
    printf("=== Benchmark Complete ===\n");
    return 0;
}
