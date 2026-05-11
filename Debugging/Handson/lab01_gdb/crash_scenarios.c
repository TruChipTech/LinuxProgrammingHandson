/*
 * crash_scenarios.c — Multiple crash scenarios for practice
 *
 * Usage: ./crash_scenarios <scenario_number>
 *   1 = NULL pointer dereference
 *   2 = Use-after-free
 *   3 = Stack overflow (recursion)
 *   4 = Double-free
 *   5 = Buffer overflow
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Scenario 1: NULL pointer dereference */
static void null_deref(void)
{
    int *ptr = NULL;
    printf("Dereferencing NULL pointer...\n");
    *ptr = 42;
}

/* Scenario 2: Use-after-free */
static void use_after_free(void)
{
    char *buf = malloc(64);
    strcpy(buf, "Hello, World!");
    printf("Before free: %s\n", buf);
    free(buf);
    printf("After free: %s\n", buf);   /* UAF */
    buf[0] = 'X';                       /* UAF write — likely crash */
}

/* Scenario 3: Stack overflow via recursion */
static int infinite_recurse(int depth)
{
    char stack_hog[4096];  /* Use lots of stack per frame */
    memset(stack_hog, 'A', sizeof(stack_hog));
    printf("Depth: %d\n", depth);
    return infinite_recurse(depth + 1);
}

/* Scenario 4: Double-free */
static void double_free(void)
{
    char *ptr = malloc(128);
    strcpy(ptr, "Double free test");
    free(ptr);
    printf("First free done\n");
    free(ptr);   /* Double-free! */
}

/* Scenario 5: Buffer overflow */
static void buffer_overflow(void)
{
    char small_buf[16];
    printf("Writing 256 bytes to 16-byte buffer...\n");
    memset(small_buf, 'A', 256);  /* Stack buffer overflow */
    printf("After overflow: %s\n", small_buf);
}

int main(int argc, char **argv)
{
    int scenario;

    if (argc < 2) {
        printf("Usage: %s <1-5>\n", argv[0]);
        printf("  1 = NULL pointer\n");
        printf("  2 = Use-after-free\n");
        printf("  3 = Stack overflow\n");
        printf("  4 = Double-free\n");
        printf("  5 = Buffer overflow\n");
        return 1;
    }

    scenario = atoi(argv[1]);

    switch (scenario) {
    case 1: null_deref(); break;
    case 2: use_after_free(); break;
    case 3: infinite_recurse(0); break;
    case 4: double_free(); break;
    case 5: buffer_overflow(); break;
    default:
        printf("Unknown scenario: %d\n", scenario);
        return 1;
    }

    return 0;
}
