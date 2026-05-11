/*
 * fake_ptrace.c — LD_PRELOAD library to bypass ptrace anti-debugging
 *
 * Build: gcc -shared -fPIC -o fake_ptrace.so fake_ptrace.c
 * Usage: LD_PRELOAD=./fake_ptrace.so ./anti_debug
 */

#include <stdio.h>

/* Override ptrace — always return success */
long ptrace(int request, ...) {
    (void)request;
    /* Silently return success so the anti-debug check passes */
    return 0;
}
