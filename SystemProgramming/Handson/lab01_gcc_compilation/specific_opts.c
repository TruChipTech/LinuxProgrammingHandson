/*
 * specific_opts.c — Demonstrate Specific GCC Optimizations
 *
 * Each function targets a specific optimization:
 *   -fdce        Dead Code Elimination
 *   -funroll-loops  Loop Unrolling
 *   -finline-functions  Function Inlining
 *   -ftree-vectorize    Vectorization
 *
 * Usage:
 *   gcc -O0 specific_opts.c -o spec_base
 *   gcc -O0 -fdce specific_opts.c -o spec_dce
 *   objdump -d spec_base | grep -A 30 "<dead_code_test>"
 *   objdump -d spec_dce  | grep -A 30 "<dead_code_test>"
 */

#include <stdio.h>
#include <stdlib.h>

/* ============================================================
 * Dead Code Elimination target
 * ============================================================ */
int dead_code_test(int x) {
    int result = x * 2;
    
    /* This block can never execute */
    if (0) {
        result = result * 100;
        result = result + 999;
        printf("This never prints\n");
    }
    
    /* Unreachable after return */
    return result;
    
    result = result * 50;  /* Dead code after return */
    return result;
}

/* ============================================================
 * Loop Unrolling target
 * ============================================================ */
int loop_test(int *arr, int n) {
    int sum = 0;
    /* Simple loop — compiler can unroll this */
    for (int i = 0; i < n; i++) {
        sum += arr[i];
    }
    return sum;
}

/* Fixed iteration loop — easier to unroll */
int fixed_loop_test(int *arr) {
    int sum = 0;
    for (int i = 0; i < 8; i++) {
        sum += arr[i];
    }
    return sum;
}

/* ============================================================
 * Function Inlining target
 * ============================================================ */
int tiny_function(int x) {
    return x + 1;
}

int inline_test(void) {
    int result = 0;
    /* Many calls to a tiny function — inlining eliminates call overhead */
    result += tiny_function(1);
    result += tiny_function(2);
    result += tiny_function(3);
    result += tiny_function(4);
    result += tiny_function(5);
    return result;
}

/* ============================================================
 * Constant Folding target
 * ============================================================ */
int constant_fold_test(void) {
    /* These can all be computed at compile time */
    int a = 10 + 20;
    int b = a * 3;
    int c = b / 2;
    int d = c - 5;
    return d;
}

/* ============================================================
 * Strength Reduction target
 * ============================================================ */
int strength_reduction_test(int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        /* i * 4 can be replaced by repeated addition */
        sum += i * 4;
    }
    return sum;
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
    printf("=== Specific Optimization Demo ===\n\n");
    
    int arr[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    
    printf("dead_code_test(10)    = %d\n", dead_code_test(10));
    printf("loop_test(arr, 8)     = %d\n", loop_test(arr, 8));
    printf("fixed_loop_test(arr)  = %d\n", fixed_loop_test(arr));
    printf("inline_test()         = %d\n", inline_test());
    printf("constant_fold_test()  = %d\n", constant_fold_test());
    printf("strength_reduction(10)= %d\n", strength_reduction_test(10));
    
    printf("\n=== Done ===\n");
    return 0;
}
