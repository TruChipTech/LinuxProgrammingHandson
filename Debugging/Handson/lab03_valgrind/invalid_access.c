/*
 * invalid_access.c — Invalid memory access demo for Valgrind
 *
 * Build: gcc -g -O0 -o invalid_access invalid_access.c
 * Run:   valgrind --track-origins=yes ./invalid_access
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Bug 1: Read past end of heap buffer */
void read_past_end(void)
{
    int *arr = malloc(10 * sizeof(int));
    for (int i = 0; i < 10; i++)
        arr[i] = i;

    /* Invalid read: arr[10] is past the allocated region */
    printf("arr[10] = %d (invalid read!)\n", arr[10]);

    free(arr);
}

/* Bug 2: Use of uninitialized memory */
void use_uninit(void)
{
    int *data = malloc(5 * sizeof(int));
    /* Only initialize some elements */
    data[0] = 10;
    data[2] = 30;

    /* Conditional based on uninitialized value */
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        if (data[i] > 0)     /* BUG: data[1], data[3], data[4] uninitialized */
            sum += data[i];
    }
    printf("Sum = %d (includes uninitialized values!)\n", sum);

    free(data);
}

/* Bug 3: Write to freed memory */
void write_after_free(void)
{
    char *msg = malloc(32);
    strcpy(msg, "Hello");
    free(msg);

    /* BUG: Writing to freed memory */
    strcpy(msg, "World");
    printf("After free write: %s\n", msg);
}

/* Bug 4: Mismatched free (malloc/free vs new/delete style) */
void mismatched_free(void)
{
    /* This is really about malloc + wrong dealloc,
       but in C we'll show free() on stack memory (wrong!) */
    char stack_buf[32];
    strcpy(stack_buf, "Stack allocated");
    printf("Stack buffer: %s\n", stack_buf);
    /* Do NOT uncomment: free(stack_buf);  — would crash */
}

int main(void)
{
    printf("=== Invalid Memory Access Demo (Valgrind) ===\n\n");

    printf("[1] Read past end:\n");
    read_past_end();

    printf("\n[2] Uninitialized memory:\n");
    use_uninit();

    printf("\n[3] Write after free:\n");
    write_after_free();

    printf("\n[4] Mismatched free (demonstration only):\n");
    mismatched_free();

    printf("\nDone.\n");
    return 0;
}
