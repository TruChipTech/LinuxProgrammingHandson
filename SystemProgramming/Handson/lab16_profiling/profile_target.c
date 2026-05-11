/*
 * profile_target.c — Code with intentional performance issues for profiling
 *
 * Build: gcc -Wall -g -pg profile_target.c -o profile_target -lm
 *        (use -pg for gprof, -g for perf/valgrind)
 *
 * Profiling:
 *   gprof:     ./profile_target && gprof profile_target gmon.out
 *   perf:      perf record ./profile_target && perf report
 *   valgrind:  valgrind --tool=callgrind ./profile_target
 *   cachegrind: valgrind --tool=cachegrind ./profile_target
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define ARRAY_SIZE (1024 * 1024)
#define ITERATIONS 100

/* ============================================================
 * Function 1: CPU-intensive computation (expected hotspot)
 * ============================================================ */
double heavy_computation(double *data, int size) {
    double result = 0.0;
    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int i = 0; i < size; i++) {
            result += sin(data[i]) * cos(data[i]) * tan(data[i] * 0.1);
        }
    }
    return result;
}

/* ============================================================
 * Function 2: Cache-unfriendly access (column-major on row-major data)
 * ============================================================ */
#define MATRIX_SIZE 1024

void cache_unfriendly(int matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    /* Column-major traversal — bad for cache */
    for (int j = 0; j < MATRIX_SIZE; j++) {
        for (int i = 0; i < MATRIX_SIZE; i++) {
            matrix[i][j] = matrix[i][j] * 2 + 1;
        }
    }
}

void cache_friendly(int matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    /* Row-major traversal — good for cache */
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            matrix[i][j] = matrix[i][j] * 2 + 1;
        }
    }
}

/* ============================================================
 * Function 3: Inefficient string operations
 * ============================================================ */
void inefficient_strings(void) {
    char *result = calloc(1, 1);
    for (int i = 0; i < 10000; i++) {
        char num[32];
        snprintf(num, sizeof(num), "%d,", i);
        size_t old_len = strlen(result);
        size_t new_len = old_len + strlen(num) + 1;
        result = realloc(result, new_len);
        strcat(result, num);  /* O(n) each time — O(n²) total */
    }
    printf("  String length: %zu\n", strlen(result));
    free(result);
}

/* ============================================================
 * Function 4: Memory allocation churn
 * ============================================================ */
void allocation_churn(void) {
    for (int i = 0; i < 100000; i++) {
        /* Allocate and immediately free — wasteful */
        int *ptr = malloc(sizeof(int) * (i % 100 + 1));
        *ptr = i;
        free(ptr);
    }
}

/* ============================================================
 * Function 5: Sorting comparison
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

int qsort_cmp(const void *a, const void *b) {
    return *(const int *)a - *(const int *)b;
}

void sorting_benchmark(void) {
    int size = 10000;
    int *arr1 = malloc(size * sizeof(int));
    int *arr2 = malloc(size * sizeof(int));

    srand(42);
    for (int i = 0; i < size; i++) {
        arr1[i] = rand() % 100000;
        arr2[i] = arr1[i];
    }

    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    bubble_sort(arr1, size);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double bubble_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                       (end.tv_nsec - start.tv_nsec) / 1000000.0;

    clock_gettime(CLOCK_MONOTONIC, &start);
    qsort(arr2, size, sizeof(int), qsort_cmp);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double qsort_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                      (end.tv_nsec - start.tv_nsec) / 1000000.0;

    printf("  Bubble sort: %.2f ms\n", bubble_ms);
    printf("  qsort:       %.2f ms\n", qsort_ms);
    printf("  Speedup:     %.1fx\n", bubble_ms / qsort_ms);

    free(arr1);
    free(arr2);
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
    printf("=== Profiling Target ===\n\n");

    /* 1. Heavy computation */
    printf("[1] Heavy computation...\n");
    double *data = malloc(ARRAY_SIZE * sizeof(double));
    for (int i = 0; i < ARRAY_SIZE; i++) {
        data[i] = (double)i / ARRAY_SIZE;
    }
    double result = heavy_computation(data, ARRAY_SIZE / 10);
    printf("  Result: %f\n", result);
    free(data);

    /* 2. Cache behavior */
    printf("\n[2] Cache behavior test...\n");
    int (*matrix)[MATRIX_SIZE] = malloc(sizeof(int[MATRIX_SIZE][MATRIX_SIZE]));
    memset(matrix, 0, sizeof(int[MATRIX_SIZE][MATRIX_SIZE]));

    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    cache_unfriendly(matrix);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double unfriendly_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                           (end.tv_nsec - start.tv_nsec) / 1000000.0;

    clock_gettime(CLOCK_MONOTONIC, &start);
    cache_friendly(matrix);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double friendly_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                         (end.tv_nsec - start.tv_nsec) / 1000000.0;

    printf("  Cache-unfriendly: %.2f ms\n", unfriendly_ms);
    printf("  Cache-friendly:   %.2f ms\n", friendly_ms);
    free(matrix);

    /* 3. String inefficiency */
    printf("\n[3] Inefficient strings...\n");
    inefficient_strings();

    /* 4. Allocation churn */
    printf("\n[4] Allocation churn...\n");
    allocation_churn();
    printf("  Done (100k alloc/free cycles)\n");

    /* 5. Sorting */
    printf("\n[5] Sorting benchmark...\n");
    sorting_benchmark();

    printf("\n=== Profile this binary with gprof, perf, or valgrind ===\n");
    return 0;
}
