/*
 * memory_leaks.c — Deliberately leaky program for Valgrind practice
 *
 * Build: gcc -Wall -g memory_leaks.c -o memory_leaks
 * Test:  valgrind --leak-check=full ./memory_leaks
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Leak 1: Direct leak — malloc without free
 * ============================================================ */
void leak_direct(void) {
    printf("[Leak 1] Direct memory leak\n");
    char *buf = malloc(256);
    snprintf(buf, 256, "This memory is never freed!");
    printf("  Allocated 256 bytes at %p\n", (void *)buf);
    /* No free(buf) — LEAKED */
}

/* ============================================================
 * Leak 2: Indirect leak — lost pointer
 * ============================================================ */
void leak_indirect(void) {
    printf("[Leak 2] Indirect leak (lost pointer)\n");
    char **array = malloc(5 * sizeof(char *));
    for (int i = 0; i < 5; i++) {
        array[i] = malloc(100);
        snprintf(array[i], 100, "String %d", i);
    }
    printf("  Allocated array of 5 strings\n");
    /* Freeing array but NOT the strings inside — indirect leak */
    free(array);
}

/* ============================================================
 * Leak 3: Realloc leak — lost original pointer on failure
 * ============================================================ */
void leak_realloc(void) {
    printf("[Leak 3] Realloc pattern (safe version shown)\n");
    char *buf = malloc(100);
    strcpy(buf, "Initial data");

    /* WRONG WAY (shown in comment):
     *   buf = realloc(buf, 200);  // If realloc fails, original is lost!
     */

    /* RIGHT WAY: */
    char *tmp = realloc(buf, 200);
    if (tmp == NULL) {
        printf("  realloc failed, original still at %p\n", (void *)buf);
        free(buf);
    } else {
        buf = tmp;
        printf("  Reallocated to 200 bytes at %p\n", (void *)buf);
    }
    free(buf);
}

/* ============================================================
 * Leak 4: Still reachable — global pointer not freed at exit
 * ============================================================ */
static char *global_buffer = NULL;

void init_global(void) {
    printf("[Leak 4] Still-reachable (global not freed at exit)\n");
    global_buffer = malloc(512);
    strcpy(global_buffer, "Global data that persists until exit");
    printf("  Global buffer at %p\n", (void *)global_buffer);
    /* Valgrind reports this as "still reachable" */
}

/* ============================================================
 * Leak 5: Use-after-free (undefined behavior)
 * ============================================================ */
void use_after_free(void) {
    printf("[Leak 5] Use-after-free (UB)\n");
    int *data = malloc(10 * sizeof(int));
    for (int i = 0; i < 10; i++) data[i] = i * 42;

    free(data);

    /* This is UB — Valgrind will catch it */
    /* Commenting out to avoid crash, but Valgrind would detect:
     * printf("  data[0] = %d (USE AFTER FREE!)\n", data[0]);
     */
    printf("  (Use-after-free line commented out — uncomment to test with Valgrind)\n");
}

/* ============================================================
 * Leak 6: Buffer overflow
 * ============================================================ */
void buffer_overflow(void) {
    printf("[Leak 6] Heap buffer overflow\n");
    char *buf = malloc(10);
    strcpy(buf, "Hello");   /* OK: 6 bytes */

    /* This overflows — Valgrind's memcheck will catch it */
    /* Commenting out to avoid crash:
     * strcpy(buf, "This string is way too long for a 10-byte buffer!");
     */
    printf("  (Overflow line commented out — uncomment to test with Valgrind)\n");
    free(buf);
}

/* ============================================================
 * Proper: No leaks (for comparison)
 * ============================================================ */
void no_leak(void) {
    printf("[Clean] Properly managed memory\n");
    char **list = malloc(3 * sizeof(char *));
    for (int i = 0; i < 3; i++) {
        list[i] = malloc(64);
        snprintf(list[i], 64, "Item %d", i);
    }
    /* Free in reverse order */
    for (int i = 0; i < 3; i++) {
        free(list[i]);
    }
    free(list);
    printf("  All memory properly freed.\n");
}

int main(void) {
    printf("=== Memory Leak Demo (for Valgrind) ===\n\n");

    leak_direct();
    leak_indirect();
    leak_realloc();
    init_global();
    use_after_free();
    buffer_overflow();
    no_leak();

    printf("\n=== Run with: valgrind --leak-check=full ./%s ===\n", "memory_leaks");
    return 0;
}
