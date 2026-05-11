/*
 * use_after_free.c — Use-after-free demo for ASan
 *
 * Build: gcc -g -fsanitize=address -fno-omit-frame-pointer -o use_after_free use_after_free.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct user {
    int id;
    char name[32];
    int score;
};

struct user *create_user(int id, const char *name, int score)
{
    struct user *u = malloc(sizeof(*u));
    if (!u) return NULL;
    u->id = id;
    strncpy(u->name, name, sizeof(u->name) - 1);
    u->name[sizeof(u->name) - 1] = '\0';
    u->score = score;
    return u;
}

void print_user(const struct user *u)
{
    /* BUG: Called after u has been freed */
    printf("User %d: %s (score: %d)\n", u->id, u->name, u->score);
}

int main(void)
{
    struct user *alice = create_user(1, "Alice", 95);
    struct user *bob = create_user(2, "Bob", 87);

    printf("=== Use-After-Free Demo ===\n\n");

    printf("Before free:\n");
    print_user(alice);
    print_user(bob);

    /* Free Alice */
    printf("\nFreeing Alice...\n");
    free(alice);

    /* BUG: Use alice after it was freed */
    printf("\nAccessing freed memory:\n");
    print_user(alice);         /* Use-after-free READ */
    alice->score = 100;        /* Use-after-free WRITE */

    free(bob);
    return 0;
}
