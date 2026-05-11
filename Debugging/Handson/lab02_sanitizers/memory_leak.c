/*
 * memory_leak.c — Memory leak demo for ASan + LeakSanitizer
 *
 * Build: gcc -g -fsanitize=address,leak -fno-omit-frame-pointer -o memory_leak memory_leak.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Leak 1: Allocates but never frees */
void leaky_function(int size)
{
    char *buf = malloc(size);
    if (!buf) return;
    memset(buf, 'X', size);
    printf("Allocated %d bytes at %p (never freed!)\n", size, (void *)buf);
    /* BUG: No free(buf) — memory leaked */
}

/* Leak 2: Overwrites the only pointer to allocated memory */
char *overwrite_leak(void)
{
    char *ptr = malloc(256);
    strcpy(ptr, "Important data");

    /* BUG: Reassign ptr without freeing old allocation */
    ptr = malloc(128);
    strcpy(ptr, "New data");

    /* The first 256-byte allocation is now leaked */
    return ptr;  /* Caller must free this */
}

/* Leak 3: Early return without cleanup */
int process_with_leak(int value)
{
    int *data = malloc(100 * sizeof(int));
    if (!data) return -1;

    for (int i = 0; i < 100; i++)
        data[i] = value + i;

    if (value < 0) {
        /* BUG: Returns without freeing data */
        return -1;
    }

    int result = data[50];
    free(data);
    return result;
}

int main(void)
{
    printf("=== Memory Leak Demo ===\n\n");

    /* Leak 1 */
    printf("[1] Direct leak:\n");
    leaky_function(1024);
    leaky_function(512);

    /* Leak 2 */
    printf("\n[2] Overwrite leak:\n");
    char *result = overwrite_leak();
    printf("Got: %s\n", result);
    free(result);  /* Only frees the second allocation */

    /* Leak 3 */
    printf("\n[3] Early-return leak:\n");
    process_with_leak(-5);   /* Leaks */
    process_with_leak(10);   /* OK */

    printf("\nProgram ending — LeakSanitizer will report leaks below:\n");
    return 0;
}
