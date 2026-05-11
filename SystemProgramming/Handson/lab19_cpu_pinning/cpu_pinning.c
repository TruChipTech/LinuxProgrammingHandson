/*
 * cpu_pinning.c — CPU Affinity & Pinning Demo
 *
 * Build: gcc -Wall -g -pthread cpu_pinning.c -o cpu_pinning
 * Usage: ./cpu_pinning
 * Note: Some operations require root or CAP_SYS_NICE
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <errno.h>
#include <sys/sysinfo.h>

/* ============================================================
 * Demo 1: Query CPU topology
 * ============================================================ */
void demo_topology(void) {
    printf("=== Demo 1: CPU Topology ===\n");

    int ncpus = get_nprocs();
    int ncpus_conf = get_nprocs_conf();

    printf("  CPUs configured: %d\n", ncpus_conf);
    printf("  CPUs online:     %d\n", ncpus);

    /* Current affinity */
    cpu_set_t mask;
    CPU_ZERO(&mask);
    if (sched_getaffinity(0, sizeof(mask), &mask) == 0) {
        printf("  Current affinity: ");
        for (int i = 0; i < ncpus_conf; i++) {
            if (CPU_ISSET(i, &mask)) printf("%d ", i);
        }
        printf("\n");
        printf("  Affinity count:   %d\n", CPU_COUNT(&mask));
    }
}

/* ============================================================
 * Demo 2: Pin process to a specific CPU
 * ============================================================ */
void demo_pin_process(void) {
    printf("\n=== Demo 2: Pin Process to CPU ===\n");

    int target_cpu = 0;
    cpu_set_t mask;

    /* Pin to CPU 0 */
    CPU_ZERO(&mask);
    CPU_SET(target_cpu, &mask);

    printf("  Pinning PID %d to CPU %d...\n", getpid(), target_cpu);
    if (sched_setaffinity(0, sizeof(mask), &mask) == 0) {
        printf("  Success! Running on CPU %d\n", sched_getcpu());
    } else {
        perror("  sched_setaffinity");
    }

    /* Restore to all CPUs */
    int ncpus = get_nprocs();
    CPU_ZERO(&mask);
    for (int i = 0; i < ncpus; i++) CPU_SET(i, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);
    printf("  Restored to all CPUs\n");
}

/* ============================================================
 * Demo 3: Pin threads to different CPUs
 * ============================================================ */
struct thread_arg {
    int cpu;
    int thread_id;
};

void *pinned_thread(void *arg) {
    struct thread_arg *ta = (struct thread_arg *)arg;

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(ta->cpu, &mask);

    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) == 0) {
        /* Do some work to verify we're on the right CPU */
        volatile long sum = 0;
        for (long i = 0; i < 10000000; i++) sum += i;

        printf("  Thread %d: pinned to CPU %d, running on CPU %d\n",
               ta->thread_id, ta->cpu, sched_getcpu());
    } else {
        printf("  Thread %d: failed to pin to CPU %d\n",
               ta->thread_id, ta->cpu);
    }
    return NULL;
}

void demo_pin_threads(void) {
    printf("\n=== Demo 3: Pin Threads to CPUs ===\n");

    int ncpus = get_nprocs();
    int nthreads = ncpus < 4 ? ncpus : 4;

    pthread_t threads[4];
    struct thread_arg args[4];

    for (int i = 0; i < nthreads; i++) {
        args[i].cpu = i % ncpus;
        args[i].thread_id = i;
        pthread_create(&threads[i], NULL, pinned_thread, &args[i]);
    }

    for (int i = 0; i < nthreads; i++) {
        pthread_join(threads[i], NULL);
    }
}

/* ============================================================
 * Demo 4: CPU_SET macro operations
 * ============================================================ */
void demo_cpuset_ops(void) {
    printf("\n=== Demo 4: cpu_set_t Operations ===\n");

    cpu_set_t set1, set2, result;

    /* Set 1: CPUs 0, 1 */
    CPU_ZERO(&set1);
    CPU_SET(0, &set1);
    CPU_SET(1, &set1);

    /* Set 2: CPUs 1, 2 */
    CPU_ZERO(&set2);
    CPU_SET(1, &set2);
    CPU_SET(2, &set2);

    printf("  Set1: {0, 1}\n");
    printf("  Set2: {1, 2}\n");

    /* AND */
    CPU_AND(&result, &set1, &set2);
    printf("  AND:  {");
    for (int i = 0; i < 4; i++)
        if (CPU_ISSET(i, &result)) printf("%d ", i);
    printf("}\n");

    /* OR */
    CPU_OR(&result, &set1, &set2);
    printf("  OR:   {");
    for (int i = 0; i < 4; i++)
        if (CPU_ISSET(i, &result)) printf("%d ", i);
    printf("}\n");

    /* XOR */
    CPU_XOR(&result, &set1, &set2);
    printf("  XOR:  {");
    for (int i = 0; i < 4; i++)
        if (CPU_ISSET(i, &result)) printf("%d ", i);
    printf("}\n");

    /* EQUAL */
    printf("  Set1 == Set2: %s\n", CPU_EQUAL(&set1, &set2) ? "yes" : "no");
}

/* ============================================================
 * Demo 5: Scheduling policy with CPU pinning
 * ============================================================ */
void demo_sched_policy(void) {
    printf("\n=== Demo 5: Scheduling Policy + Pinning ===\n");

    /* Show current scheduling policy */
    int policy = sched_getscheduler(0);
    struct sched_param param;
    sched_getparam(0, &param);

    const char *policy_names[] = {"SCHED_OTHER", "SCHED_FIFO", "SCHED_RR"};
    printf("  Current policy: %s (priority %d)\n",
           policy < 3 ? policy_names[policy] : "unknown", param.sched_priority);
    printf("  Priority range for SCHED_FIFO: %d - %d\n",
           sched_get_priority_min(SCHED_FIFO),
           sched_get_priority_max(SCHED_FIFO));
    printf("  Priority range for SCHED_RR:   %d - %d\n",
           sched_get_priority_min(SCHED_RR),
           sched_get_priority_max(SCHED_RR));

    /* Try setting SCHED_FIFO (requires root) */
    param.sched_priority = 10;
    if (sched_setscheduler(0, SCHED_FIFO, &param) == 0) {
        printf("  Set SCHED_FIFO priority 10 — OK\n");
        /* Restore */
        param.sched_priority = 0;
        sched_setscheduler(0, SCHED_OTHER, &param);
    } else {
        printf("  Cannot set SCHED_FIFO (need root/CAP_SYS_NICE)\n");
    }
}

int main(void) {
    printf("=== CPU Pinning & Affinity Demo ===\n\n");

    demo_topology();
    demo_pin_process();
    demo_pin_threads();
    demo_cpuset_ops();
    demo_sched_policy();

    printf("\n=== Demo Complete ===\n");
    return 0;
}
