/*
 * pgo_demo.c — Profile-Guided Optimization Demonstration
 *
 * This program has heavily biased branches that PGO can optimize.
 *
 * PGO Workflow:
 *   1. gcc -O2 -fprofile-generate pgo_demo.c -o pgo_instrument -lm
 *   2. ./pgo_instrument                   (generates .gcda files)
 *   3. gcc -O2 -fprofile-use pgo_demo.c -o pgo_optimized -lm
 *   4. Compare: time ./pgo_optimized  vs  time ./pgo_plain
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define NUM_ITERATIONS  200000000
#define ARRAY_SIZE      100000

/* ============================================================
 * Biased Branch Function
 * The 'rare' path is taken only ~2% of the time.
 * PGO will optimize by predicting the common path.
 * ============================================================ */
int process_value(int value) {
    /* Common path (98% of the time) */
    if (value % 50 != 0) {
        return value * 2 + 1;
    }
    
    /* Rare path (2% of the time) — expensive computation */
    int result = 0;
    for (int i = 0; i < 10; i++) {
        result += value / (i + 1);
    }
    return result;
}

/* ============================================================
 * Switch-based Dispatcher
 * Some cases are much more frequent than others.
 * PGO reorders cases by frequency.
 * ============================================================ */
int dispatch_command(int cmd, int data) {
    switch (cmd) {
        case 0:  return data + 1;          /* ~40% frequency */
        case 1:  return data - 1;          /* ~30% frequency */
        case 2:  return data * 2;          /* ~15% frequency */
        case 3:  return data / 2;          /* ~10% frequency */
        case 4:  return data ^ 0xFF;       /* ~3% frequency */
        case 5:  return data & 0x0F;       /* ~1.5% frequency */
        case 6:  return ~data;             /* ~0.5% frequency */
        default: return data;              /* ~0% */
    }
}

/* ============================================================
 * Hot/Cold Function Pattern
 * hot_path is called millions of times, cold_path rarely.
 * PGO marks hot_path for aggressive optimization.
 * ============================================================ */
int hot_path(int x) {
    return x * x + x + 1;
}

int cold_path(int x) {
    double val = sqrt((double)x);
    return (int)(val * 100.0) + x;
}

/* ============================================================
 * Loop with Predictable Trip Count
 * PGO can optimize loop based on observed iteration counts.
 * ============================================================ */
long compute_series(int n) {
    long sum = 0;
    for (int i = 0; i < n; i++) {
        if (i % 3 == 0) {
            sum += i * 2;
        } else if (i % 3 == 1) {
            sum += i;
        } else {
            sum -= i / 2;
        }
    }
    return sum;
}

/* ============================================================
 * MAIN — Representative Workload for Profiling
 * ============================================================ */
int main(void) {
    printf("=== PGO Benchmark ===\n");
    
    long result = 0;
    
    /* Workload 1: Biased branches */
    printf("[1/4] Biased branch workload...\n");
    for (long i = 0; i < NUM_ITERATIONS; i++) {
        result += process_value((int)(i & 0xFFFF));
    }
    printf("  Result: %ld\n", result);
    
    /* Workload 2: Biased switch dispatch */
    printf("[2/4] Switch dispatch workload...\n");
    result = 0;
    for (long i = 0; i < NUM_ITERATIONS; i++) {
        /* Generate commands with biased distribution */
        int cmd;
        int r = (int)(i % 200);
        if (r < 80)       cmd = 0;   /* 40% */
        else if (r < 140)  cmd = 1;   /* 30% */
        else if (r < 170)  cmd = 2;   /* 15% */
        else if (r < 190)  cmd = 3;   /* 10% */
        else if (r < 196)  cmd = 4;   /* 3% */
        else if (r < 199)  cmd = 5;   /* 1.5% */
        else               cmd = 6;   /* 0.5% */
        
        result += dispatch_command(cmd, (int)(i & 0xFF));
    }
    printf("  Result: %ld\n", result);
    
    /* Workload 3: Hot/Cold function calls */
    printf("[3/4] Hot/cold path workload...\n");
    result = 0;
    for (long i = 0; i < NUM_ITERATIONS; i++) {
        if (i % 1000 != 0) {
            result += hot_path((int)(i & 0xFF));   /* 99.9% hot */
        } else {
            result += cold_path((int)(i & 0xFF));   /* 0.1% cold */
        }
    }
    printf("  Result: %ld\n", result);
    
    /* Workload 4: Series computation */
    printf("[4/4] Series computation...\n");
    result = 0;
    for (int i = 0; i < 2000; i++) {
        result += compute_series(ARRAY_SIZE);
    }
    printf("  Result: %ld\n", result);
    
    printf("=== Benchmark Complete ===\n");
    return 0;
}
