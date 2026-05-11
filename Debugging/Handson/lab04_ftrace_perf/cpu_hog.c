/*
 * cpu_hog.c — CPU-intensive program for perf profiling
 *
 * Build: gcc -g -O2 -o cpu_hog cpu_hog.c -lm
 * Profile: sudo perf record -g ./cpu_hog && sudo perf report
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define ARRAY_SIZE 10000000

/* Hot function 1: lots of computation */
double compute_heavy(double *data, int size)
{
    double result = 0.0;
    for (int i = 0; i < size; i++) {
        result += sin(data[i]) * cos(data[i]);
    }
    return result;
}

/* Hot function 2: memory-intensive (cache misses) */
long sum_with_stride(int *arr, int size, int stride)
{
    long sum = 0;
    for (int i = 0; i < size; i += stride) {
        sum += arr[i];
    }
    return sum;
}

/* Hot function 3: branch-heavy */
int count_threshold(int *arr, int size, int threshold)
{
    int count = 0;
    for (int i = 0; i < size; i++) {
        if (arr[i] > threshold)
            count++;
    }
    return count;
}

int main(void)
{
    double *fdata = malloc(ARRAY_SIZE * sizeof(double));
    int *idata = malloc(ARRAY_SIZE * sizeof(int));

    /* Initialize */
    for (int i = 0; i < ARRAY_SIZE; i++) {
        fdata[i] = (double)i / ARRAY_SIZE * 3.14159;
        idata[i] = rand() % 1000;
    }

    printf("Running compute_heavy...\n");
    double r1 = compute_heavy(fdata, ARRAY_SIZE);

    printf("Running sum_with_stride (stride=1, cache-friendly)...\n");
    long r2 = sum_with_stride(idata, ARRAY_SIZE, 1);

    printf("Running sum_with_stride (stride=64, cache-unfriendly)...\n");
    long r3 = sum_with_stride(idata, ARRAY_SIZE, 64);

    printf("Running count_threshold...\n");
    int r4 = count_threshold(idata, ARRAY_SIZE, 500);

    printf("Results: %.2f, %ld, %ld, %d\n", r1, r2, r3, r4);

    free(fdata);
    free(idata);
    return 0;
}
