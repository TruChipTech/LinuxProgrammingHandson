/*
 * sched_demo.c — Process Scheduling Policies Demo
 *
 * Build: gcc -Wall -g sched_demo.c -o sched_demo -lpthread
 * Run:   sudo ./sched_demo   (real-time policies require root)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>

static const char *policy_name(int policy) {
    switch (policy) {
        case SCHED_OTHER: return "SCHED_OTHER (CFS)";
        case SCHED_FIFO:  return "SCHED_FIFO";
        case SCHED_RR:    return "SCHED_RR";
        case SCHED_BATCH: return "SCHED_BATCH";
        case SCHED_IDLE:  return "SCHED_IDLE";
        default:          return "Unknown";
    }
}

/* ============================================================
 * Demo 1: Query current scheduling
 * ============================================================ */
void demo_query_sched(void) {
    printf("--- Demo 1: Current Scheduling Info ---\n");

    int policy = sched_getscheduler(0);
    struct sched_param param;
    sched_getparam(0, &param);

    printf("  PID:      %d\n", getpid());
    printf("  Policy:   %s\n", policy_name(policy));
    printf("  Priority: %d\n", param.sched_priority);
    printf("  Nice:     %d\n", getpriority(PRIO_PROCESS, 0));

    /* Show valid priority ranges */
    printf("\n  Priority ranges:\n");
    int policies[] = {SCHED_OTHER, SCHED_FIFO, SCHED_RR, SCHED_BATCH, SCHED_IDLE};
    for (int i = 0; i < 5; i++) {
        printf("    %-20s min=%d max=%d\n", policy_name(policies[i]),
               sched_get_priority_min(policies[i]),
               sched_get_priority_max(policies[i]));
    }

    /* Time quantum for RR */
    struct timespec ts;
    if (sched_rr_get_interval(0, &ts) == 0) {
        printf("\n  RR time quantum: %ld ms\n",
               ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    }
}

/* ============================================================
 * Demo 2: Nice values
 * ============================================================ */
void demo_nice(void) {
    printf("\n--- Demo 2: Nice Values ---\n");

    printf("  Current nice: %d\n", getpriority(PRIO_PROCESS, 0));

    /* Increase nice (lower priority) — doesn't need root */
    errno = 0;
    int new_nice = nice(5);
    if (errno == 0) {
        printf("  After nice(5): %d\n", new_nice);
    }

    /* Try to decrease nice (needs root) */
    errno = 0;
    nice(-5);
    if (errno != 0) {
        printf("  nice(-5): Permission denied (need root)\n");
    } else {
        printf("  After nice(-5): %d\n", getpriority(PRIO_PROCESS, 0));
    }
}

/* ============================================================
 * Demo 3: Real-time scheduling
 * ============================================================ */
void demo_realtime(void) {
    printf("\n--- Demo 3: Real-Time Scheduling ---\n");

    if (geteuid() != 0) {
        printf("  [SKIP] Requires root. Run with: sudo %s\n", "sched_demo");
        return;
    }

    /* Set SCHED_FIFO */
    struct sched_param param = { .sched_priority = 50 };
    if (sched_setscheduler(0, SCHED_FIFO, &param) == 0) {
        printf("  Set SCHED_FIFO with priority 50\n");
    } else {
        perror("  sched_setscheduler FIFO");
    }

    /* Verify */
    int pol = sched_getscheduler(0);
    sched_getparam(0, &param);
    printf("  Current: %s, priority=%d\n",
           policy_name(pol), param.sched_priority);

    /* Switch to SCHED_RR */
    param.sched_priority = 30;
    if (sched_setscheduler(0, SCHED_RR, &param) == 0) {
        printf("  Switched to SCHED_RR with priority 30\n");
    }

    /* Back to normal */
    param.sched_priority = 0;
    sched_setscheduler(0, SCHED_OTHER, &param);
    printf("  Back to SCHED_OTHER\n");
}

/* ============================================================
 * Demo 4: Priority competition
 * ============================================================ */
void cpu_burn(const char *label, int iterations) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    volatile long sum = 0;
    for (int i = 0; i < iterations; i++) {
        sum += i;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                (end.tv_nsec - start.tv_nsec) / 1000000.0;
    printf("  [%s] %d iterations in %.1f ms (sum=%ld)\n",
           label, iterations, ms, (long)sum);
}

void demo_priority_competition(void) {
    printf("\n--- Demo 4: Priority Competition ---\n");

    int iters = 50000000;

    for (int nice_val = -10; nice_val <= 10; nice_val += 10) {
        pid_t pid = fork();
        if (pid == 0) {
            /* Only set negative nice if root */
            if (nice_val < 0 && geteuid() == 0) {
                nice(nice_val);
            } else if (nice_val >= 0) {
                nice(nice_val);
            }

            char label[32];
            snprintf(label, sizeof(label), "nice=%d", nice_val);
            cpu_burn(label, iters);
            _exit(0);
        }
    }

    /* Wait for all children */
    while (wait(NULL) > 0);
}

int main(void) {
    printf("=== Process Scheduling Demo ===\n\n");

    demo_query_sched();
    demo_nice();
    demo_realtime();
    demo_priority_competition();

    printf("\n=== Demo Complete ===\n");
    return 0;
}
