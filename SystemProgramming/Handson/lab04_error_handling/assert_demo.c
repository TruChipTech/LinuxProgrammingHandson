/*
 * assert_demo.c — Assertions and Defensive Programming
 *
 * Debug:   gcc -Wall -g -DDEBUG assert_demo.c -o assert_debug
 * Release: gcc -Wall -O2 -DNDEBUG assert_demo.c -o assert_release
 *
 * Trigger assertion: ./assert_debug trigger_assert
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================
 * Custom assertion macros
 * ============================================================ */

#ifndef NDEBUG

#define ASSERT_MSG(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "ASSERTION FAILED: %s\n  %s:%d in %s()\n  Message: %s\n", \
                #cond, __FILE__, __LINE__, __func__, (msg)); \
        abort(); \
    } \
} while (0)

#define ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        fprintf(stderr, "NULL POINTER: %s is NULL\n  %s:%d in %s()\n", \
                #ptr, __FILE__, __LINE__, __func__); \
        abort(); \
    } \
} while (0)

#define PRECONDITION(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "PRECONDITION VIOLATED: %s\n  %s:%d in %s()\n", \
                #cond, __FILE__, __LINE__, __func__); \
        abort(); \
    } \
} while (0)

#define POSTCONDITION(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "POSTCONDITION VIOLATED: %s\n  %s:%d in %s()\n", \
                #cond, __FILE__, __LINE__, __func__); \
        abort(); \
    } \
} while (0)

#else /* NDEBUG — release build: all assertions become no-ops */

#define ASSERT_MSG(cond, msg)   ((void)0)
#define ASSERT_NOT_NULL(ptr)    ((void)0)
#define PRECONDITION(cond)      ((void)0)
#define POSTCONDITION(cond)     ((void)0)

#endif

/* ============================================================
 * C11 static_assert (compile-time check)
 * ============================================================ */
_Static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");
_Static_assert(sizeof(void *) == 8, "Expected 64-bit pointers");

/* ============================================================
 * Functions with assertions
 * ============================================================ */

/* Division with precondition */
double safe_divide(double a, double b) {
    PRECONDITION(b != 0.0);
    double result = a / b;
    return result;
}

/* Array access with bounds checking */
int safe_array_get(const int *arr, int size, int index) {
    ASSERT_NOT_NULL(arr);
    ASSERT_MSG(index >= 0 && index < size,
               "Array index out of bounds");
    return arr[index];
}

/* Buffer copy with size assertion */
void safe_copy(char *dst, size_t dst_size, const char *src) {
    ASSERT_NOT_NULL(dst);
    ASSERT_NOT_NULL(src);
    PRECONDITION(dst_size > 0);

    size_t src_len = strlen(src);
    ASSERT_MSG(src_len < dst_size,
               "Source string too long for destination buffer");

    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';

    POSTCONDITION(strlen(dst) <= src_len);
}

/* Factorial with range assertion */
int factorial(int n) {
    PRECONDITION(n >= 0);
    ASSERT_MSG(n <= 20, "Input too large, would overflow int");

    int result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }

    POSTCONDITION(result > 0);
    return result;
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(int argc, char *argv[]) {
    printf("=== Assertion Demo ===\n");

#ifdef NDEBUG
    printf("Build: RELEASE (assertions disabled)\n\n");
#else
    printf("Build: DEBUG (assertions enabled)\n\n");
#endif

    /* Standard assert */
    printf("--- Standard assert() ---\n");
    int x = 42;
    assert(x > 0);
    printf("  assert(x > 0) passed (x=%d)\n", x);

    /* Custom assertions — normal usage */
    printf("\n--- Custom assertions (passing) ---\n");

    double result = safe_divide(10.0, 3.0);
    printf("  safe_divide(10, 3) = %.4f\n", result);

    int arr[] = {10, 20, 30, 40, 50};
    printf("  safe_array_get(arr, 5, 2) = %d\n",
           safe_array_get(arr, 5, 2));

    char buf[32];
    safe_copy(buf, sizeof(buf), "Hello Assertions!");
    printf("  safe_copy: '%s'\n", buf);

    printf("  factorial(10) = %d\n", factorial(10));

    /* Trigger assertion failure if requested */
    if (argc > 1 && strcmp(argv[1], "trigger_assert") == 0) {
        printf("\n--- Triggering assertion failure ---\n");
        printf("Calling safe_divide(1.0, 0.0)...\n");
        safe_divide(1.0, 0.0);  /* PRECONDITION will fire */
    }

    printf("\n=== Demo Complete ===\n");
    return 0;
}
