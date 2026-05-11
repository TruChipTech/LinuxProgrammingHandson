/*
 * anti_debug.c — Anti-Debugging Techniques Demo
 *
 * Build: gcc -Wall -g anti_debug.c -o anti_debug
 *
 * Implements multiple anti-debugging layers:
 *   1. ptrace self-trace
 *   2. /proc/self/status TracerPid check
 *   3. Timing-based detection
 *   4. Signal-based detection
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <time.h>
#include <signal.h>

/* ============================================================
 * Layer 1: ptrace TRACEME — debugger can't attach if we trace ourselves
 * ============================================================ */
int check_ptrace(void) {
    if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
        return 1;  /* Debugger detected: ptrace already attached */
    }
    /* Detach from self */
    ptrace(PTRACE_DETACH, 0, NULL, NULL);
    return 0;
}

/* ============================================================
 * Layer 2: /proc/self/status — check TracerPid
 * ============================================================ */
int check_proc_status(void) {
    FILE *fp = fopen("/proc/self/status", "r");
    if (!fp) return 0;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "TracerPid:", 10) == 0) {
            int tracer_pid = atoi(line + 10);
            fclose(fp);
            return (tracer_pid != 0) ? 1 : 0;
        }
    }

    fclose(fp);
    return 0;
}

/* ============================================================
 * Layer 3: Timing check — debugger breakpoints cause delays
 * ============================================================ */
int check_timing(void) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    /* Do some work that shouldn't take long */
    volatile int sum = 0;
    for (int i = 0; i < 100000; i++) {
        sum += i;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000L +
                      (end.tv_nsec - start.tv_nsec);

    /* If this simple loop takes more than 100ms, something is wrong */
    if (elapsed_ns > 100000000L) {
        return 1;  /* Suspicious delay — possible breakpoint */
    }
    return 0;
}

/* ============================================================
 * Layer 4: Signal-based detection
 * ============================================================ */
static volatile int signal_received = 0;

void trap_handler(int sig) {
    (void)sig;
    signal_received = 1;
}

int check_signal(void) {
    signal_received = 0;
    signal(SIGTRAP, trap_handler);

    /* Raise SIGTRAP — debugger will intercept this */
    raise(SIGTRAP);

    /* If we get here and signal_received is 1, no debugger */
    /* If debugger caught it, signal_received stays 0 */
    signal(SIGTRAP, SIG_DFL);
    return (signal_received == 0) ? 1 : 0;
}

/* ============================================================
 * The protected function — only runs if no debugger
 * ============================================================ */
void protected_operation(void) {
    printf("\n[PROTECTED] Executing sensitive operation...\n");
    printf("[PROTECTED] Secret key: 0x%X%X%X%X\n",
           0xDE, 0xAD, 0xBE, 0xEF);
    printf("[PROTECTED] Operation complete.\n");
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
    printf("=== Anti-Debugging Demo ===\n\n");

    int debug_detected = 0;

    /* Layer 1: ptrace check */
    printf("[CHECK 1] ptrace self-trace... ");
    if (check_ptrace()) {
        printf("DEBUGGER DETECTED!\n");
        debug_detected = 1;
    } else {
        printf("OK\n");
    }

    /* Layer 2: /proc/self/status check */
    printf("[CHECK 2] /proc/self/status... ");
    if (check_proc_status()) {
        printf("DEBUGGER DETECTED!\n");
        debug_detected = 1;
    } else {
        printf("OK\n");
    }

    /* Layer 3: Timing check */
    printf("[CHECK 3] Timing analysis...   ");
    if (check_timing()) {
        printf("SUSPICIOUS TIMING!\n");
        debug_detected = 1;
    } else {
        printf("OK\n");
    }

    /* Layer 4: Signal check */
    printf("[CHECK 4] Signal trap...       ");
    if (check_signal()) {
        printf("DEBUGGER DETECTED!\n");
        debug_detected = 1;
    } else {
        printf("OK\n");
    }

    printf("\n");

    if (debug_detected) {
        printf("[!] Debugging detected! Aborting sensitive operation.\n");
        return 1;
    }

    protected_operation();

    printf("\n=== Demo Complete ===\n");
    return 0;
}
