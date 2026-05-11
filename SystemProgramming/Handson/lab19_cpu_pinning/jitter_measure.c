/*
 * jitter_measure.c — CPU Jitter Measurement Tool
 *
 * Measures loop-timing jitter with and without CPU pinning.
 * Build: gcc -Wall -O2 -pthread jitter_measure.c -o jitter_measure -lrt
 * Usage: ./jitter_measure [cpu_number]
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <float.h>

#define ITERATIONS  10000
#define HIST_BUCKETS 20

typedef struct {
    double min_ns, max_ns, avg_ns, stddev_ns;
    double p50, p99;
    int histogram[HIST_BUCKETS];
} jitter_result_t;

static int cmp_double(const void *a, const void *b) {
    double da = *(const double *)a, db = *(const double *)b;
    return (da > db) - (da < db);
}

/* Measure iteration jitter */
static void measure_jitter(jitter_result_t *result) {
    double samples[ITERATIONS];
    struct timespec t1, t2;

    /* Warmup */
    for (int i = 0; i < 100; i++) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
    }

    /* Measure */
    for (int i = 0; i < ITERATIONS; i++) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &t1);

        /* Simulate a fixed-time workload */
        volatile long sum = 0;
        for (int j = 0; j < 1000; j++) sum += j;

        clock_gettime(CLOCK_MONOTONIC_RAW, &t2);

        samples[i] = (t2.tv_sec - t1.tv_sec) * 1e9 +
                      (t2.tv_nsec - t1.tv_nsec);
    }

    /* Statistics */
    qsort(samples, ITERATIONS, sizeof(double), cmp_double);

    result->min_ns = samples[0];
    result->max_ns = samples[ITERATIONS - 1];
    result->p50 = samples[ITERATIONS / 2];
    result->p99 = samples[(int)(ITERATIONS * 0.99)];

    double total = 0;
    for (int i = 0; i < ITERATIONS; i++) total += samples[i];
    result->avg_ns = total / ITERATIONS;

    double var = 0;
    for (int i = 0; i < ITERATIONS; i++) {
        double d = samples[i] - result->avg_ns;
        var += d * d;
    }
    result->stddev_ns = sqrt(var / ITERATIONS);

    /* Build histogram */
    double range = result->max_ns - result->min_ns;
    double bucket_size = range / HIST_BUCKETS;
    memset(result->histogram, 0, sizeof(result->histogram));
    for (int i = 0; i < ITERATIONS; i++) {
        int bucket = (int)((samples[i] - result->min_ns) / bucket_size);
        if (bucket >= HIST_BUCKETS) bucket = HIST_BUCKETS - 1;
        result->histogram[bucket]++;
    }
}

static void print_result(const char *label, const jitter_result_t *r) {
    printf("\n=== %s ===\n", label);
    printf("  Min:     %.0f ns\n", r->min_ns);
    printf("  Max:     %.0f ns\n", r->max_ns);
    printf("  Avg:     %.0f ns\n", r->avg_ns);
    printf("  Stddev:  %.0f ns\n", r->stddev_ns);
    printf("  P50:     %.0f ns\n", r->p50);
    printf("  P99:     %.0f ns\n", r->p99);
    printf("  Jitter:  %.0f ns (max - min)\n", r->max_ns - r->min_ns);

    /* Print histogram */
    printf("\n  Histogram (%d iterations):\n", ITERATIONS);
    int max_count = 0;
    for (int i = 0; i < HIST_BUCKETS; i++)
        if (r->histogram[i] > max_count) max_count = r->histogram[i];

    double range = r->max_ns - r->min_ns;
    double bucket_size = range / HIST_BUCKETS;
    for (int i = 0; i < HIST_BUCKETS; i++) {
        double lo = r->min_ns + i * bucket_size;
        double hi = lo + bucket_size;
        int bar = max_count > 0 ? (r->histogram[i] * 40) / max_count : 0;
        printf("  %6.0f-%6.0f | ", lo, hi);
        for (int j = 0; j < bar; j++) printf("#");
        printf(" %d\n", r->histogram[i]);
    }
}

/* Needed for sqrt — link with -lm or use inline */
static double sqrt(double x) {
    if (x <= 0) return 0;
    double guess = x / 2;
    for (int i = 0; i < 50; i++)
        guess = (guess + x / guess) / 2;
    return guess;
}

int main(int argc, char *argv[]) {
    int target_cpu = argc > 1 ? atoi(argv[1]) : 0;
    jitter_result_t unpinned, pinned;

    printf("=== CPU Jitter Measurement ===\n");
    printf("Iterations: %d\n", ITERATIONS);
    printf("Target CPU: %d\n", target_cpu);

    /* Measure without pinning */
    printf("\nMeasuring without CPU pinning...\n");
    measure_jitter(&unpinned);
    print_result("Without CPU Pinning", &unpinned);

    /* Pin to specific CPU */
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(target_cpu, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask) != 0) {
        perror("sched_setaffinity");
        printf("Could not pin to CPU %d\n", target_cpu);
        return 1;
    }

    printf("\nMeasuring with CPU pinning (CPU %d)...\n", target_cpu);
    measure_jitter(&pinned);
    print_result("With CPU Pinning", &pinned);

    /* Comparison */
    printf("\n=== Comparison ===\n");
    printf("  Jitter reduction: %.0f ns -> %.0f ns (%.1f%%)\n",
           unpinned.max_ns - unpinned.min_ns,
           pinned.max_ns - pinned.min_ns,
           100.0 * (1.0 - (pinned.max_ns - pinned.min_ns) /
                          (unpinned.max_ns - unpinned.min_ns)));
    printf("  Stddev reduction: %.0f ns -> %.0f ns (%.1f%%)\n",
           unpinned.stddev_ns, pinned.stddev_ns,
           100.0 * (1.0 - pinned.stddev_ns / unpinned.stddev_ns));

    return 0;
}
