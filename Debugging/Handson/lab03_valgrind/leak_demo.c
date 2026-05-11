/*
 * leak_demo.c — Memory leak demo for Valgrind Memcheck
 *
 * Build: gcc -g -O0 -o leak_demo leak_demo.c
 * Run:   valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./leak_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node {
    int value;
    struct node *next;
};

/* Leak: builds a linked list but never frees it */
struct node *build_list(int count)
{
    struct node *head = NULL;
    for (int i = 0; i < count; i++) {
        struct node *n = malloc(sizeof(*n));
        n->value = i;
        n->next = head;
        head = n;
    }
    return head;
}

/* Leak: allocates inside a loop, only keeps last pointer */
void loop_leak(int iterations)
{
    char *data = NULL;
    for (int i = 0; i < iterations; i++) {
        data = malloc(128);               /* Previous allocation leaked! */
        snprintf(data, 128, "iteration %d", i);
    }
    /* Only the last allocation is reachable */
    printf("Last: %s\n", data);
    free(data);
}

/* Conditional leak: leaks only on error path */
int process_file(const char *filename)
{
    char *buffer = malloc(1024);
    if (!buffer) return -1;

    FILE *f = fopen(filename, "r");
    if (!f) {
        /* BUG: Returns without freeing buffer */
        fprintf(stderr, "Cannot open %s\n", filename);
        return -1;
    }

    fread(buffer, 1, 1024, f);
    fclose(f);
    free(buffer);
    return 0;
}

int main(void)
{
    printf("=== Valgrind Leak Demo ===\n\n");

    /* Leak 1: Linked list never freed */
    printf("[1] Building leaked linked list...\n");
    struct node *list = build_list(5);
    /* BUG: list is never freed — 5 * sizeof(struct node) leaked */
    (void)list;  /* Suppress unused warning */

    /* Leak 2: Loop allocation leak */
    printf("[2] Loop leak...\n");
    loop_leak(10);

    /* Leak 3: Error-path leak */
    printf("[3] Error path leak...\n");
    process_file("/nonexistent/file.txt");

    printf("\nDone. Run with valgrind to see leak report.\n");
    return 0;
}
