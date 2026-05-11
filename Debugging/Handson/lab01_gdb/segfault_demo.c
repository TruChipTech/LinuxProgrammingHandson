/*
 * segfault_demo.c — Intentional segfault for core dump analysis
 *
 * Build: make
 * Run:   ulimit -c unlimited && ./segfault_demo
 * Debug: gdb ./segfault_demo core.*
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct record {
    int id;
    char name[32];
    struct record *next;
};

static struct record *find_record(struct record *head, int id)
{
    struct record *cur = head;
    while (cur != NULL) {
        if (cur->id == id)
            return cur;
        cur = cur->next;
    }
    return NULL;
}

static void process_record(struct record *rec)
{
    /* BUG: rec can be NULL if find_record returns NULL */
    printf("Processing record %d: %s\n", rec->id, rec->name);
}

int main(void)
{
    struct record r1 = { .id = 1, .name = "Alice", .next = NULL };
    struct record r2 = { .id = 2, .name = "Bob", .next = &r1 };
    struct record *head = &r2;
    struct record *found;

    printf("Looking for record 3 (does not exist)...\n");

    found = find_record(head, 3);   /* Returns NULL */
    process_record(found);           /* SEGFAULT: NULL dereference */

    return 0;
}
