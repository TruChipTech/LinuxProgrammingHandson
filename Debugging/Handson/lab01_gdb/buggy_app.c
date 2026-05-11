/*
 * buggy_app.c — Multi-function program with intentional bugs
 *
 * Contains several bugs for GDB debugging practice:
 *   - Off-by-one in array access
 *   - Null pointer dereference under certain conditions
 *   - Integer overflow
 *
 * Build: make
 * Debug: gdb ./buggy_app
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_SIZE 10

/* Bug 1: Off-by-one — accesses array[ARRAY_SIZE] */
int process_data(int *data, int count)
{
    int sum = 0;
    /* BUG: should be i < count, not i <= count */
    for (int i = 0; i <= count; i++) {
        sum += data[i];
    }
    return sum;
}

/* Bug 2: Null pointer dereference when name is NULL */
void greet_user(const char *name)
{
    char greeting[64];
    /* BUG: no NULL check before strlen */
    snprintf(greeting, sizeof(greeting), "Hello, %s! (len=%zu)",
             name, strlen(name));
    printf("%s\n", greeting);
}

/* Bug 3: Integer overflow */
int compute(int a, int b)
{
    /* BUG: no overflow check */
    int result = a * b;
    printf("compute(%d, %d) = %d\n", a, b, result);
    return result;
}

/* Bug 4: Use-after-free */
char *create_message(const char *text)
{
    char *msg = malloc(strlen(text) + 20);
    if (!msg) return NULL;
    sprintf(msg, "Message: %s", text);
    return msg;
}

int main(int argc, char **argv)
{
    int data[ARRAY_SIZE];
    int sum;
    char *msg;

    printf("=== Buggy App — Debug me with GDB! ===\n\n");

    /* Initialize array */
    for (int i = 0; i < ARRAY_SIZE; i++)
        data[i] = i * 10;

    /* Bug 1: Off-by-one */
    printf("[1] Processing data array...\n");
    sum = process_data(data, ARRAY_SIZE);
    printf("Sum = %d\n\n", sum);

    /* Bug 2: NULL pointer (only triggers with no args) */
    printf("[2] Greeting user...\n");
    if (argc > 1) {
        greet_user(argv[1]);
    } else {
        greet_user(NULL);  /* Will crash! */
    }
    printf("\n");

    /* Bug 3: Integer overflow */
    printf("[3] Computing...\n");
    compute(100000, 100000);
    printf("\n");

    /* Bug 4: Use-after-free */
    printf("[4] Creating message...\n");
    msg = create_message("Hello, World");
    printf("%s\n", msg);
    free(msg);
    /* BUG: Using msg after free */
    printf("After free: %s\n", msg);

    return 0;
}
