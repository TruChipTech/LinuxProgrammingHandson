/*
 * heap_overflow.c — Heap buffer overflow demo for ASan
 *
 * Build without ASan:  gcc -g -o heap_overflow_unsafe heap_overflow.c
 * Build with ASan:     gcc -g -fsanitize=address -fno-omit-frame-pointer -o heap_overflow heap_overflow.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void overflow_function(void)
{
    int *buf = malloc(10 * sizeof(int));  /* 40 bytes */
    if (!buf) return;

    printf("Allocated 10 ints (%zu bytes)\n", 10 * sizeof(int));

    /* Fill the buffer */
    for (int i = 0; i < 10; i++)
        buf[i] = i * 100;

    /* BUG: Write past the end of the buffer */
    printf("Writing past end of buffer...\n");
    buf[10] = 9999;   /* Heap buffer overflow! */
    buf[11] = 8888;   /* Even further out */

    printf("buf[10] = %d (out of bounds!)\n", buf[10]);

    free(buf);
}

int main(void)
{
    printf("=== Heap Buffer Overflow Demo ===\n");
    printf("Without ASan, this may appear to 'work' silently.\n");
    printf("With ASan, it will be caught immediately.\n\n");

    overflow_function();

    printf("Program completed (if you see this, the bug was silent!)\n");
    return 0;
}
