/*
 * thread_sched.c — Thread Scheduling Policies & Priority Inversion
 *
 * Build: gcc -Wall -g thread_sched.c -o thread_sched -lpthread
 * Run:   sudo ./thread_sched
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <errno.h>

static const char *policy_str(int p) {
    switch (p) {
        case SCHED_OTHER: return "OTHER";
        case SCHED_FIFO:  return "FIFO";
        case SCHED_RR:    return "RR";
        default:          return "???";
    }
}

/* ============================================================
 * Demo 1: Per-thread scheduling
 * ============================================================ */
typedef struct {
    int id;
    int policy;
    int priority;
} thread_info_t;

void *sched_thread(void *arg) {
    thread_info_t *info = (thread_info_t *)arg;

    int actual_policy;
    struct sched_param actual_param;
    pthread_getschedparam(pthread_self(), &actual_policy, &actual_param);

    printf("  [Thread %d] Policy=%-5s Priority=%d (requested: %s/%d)\n",
           info->id, policy_str(actual_policy), actual_param.sched_priority,
           policy_str(info->policy), info->priority);

    /* CPU burn to show scheduling effect */
    volatile long sum = 0;
    for (long i = 0; i < 20000000L; i++) sum += i;

    return NULL;
}

void demo_thread_policies(void) {
    printf("--- Demo 1: Per-Thread Scheduling Policies ---\n");

    if (geteuid() != 0) {
        printf("  [SKIP] Requires root.\n");
        return;
    }

    thread_info_t configs[] = {
        {0, SCHED_OTHER, 0},
        {1, SCHED_FIFO,  10},
        {2, SCHED_FIFO,  50},
        {3, SCHED_RR,    30},
    };
    int n = sizeof(configs) / sizeof(configs[0]);

    pthread_t threads[4];
    for (int i = 0; i < n; i++) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&attr, configs[i].policy);

        struct sched_param param = { .sched_priority = configs[i].priority };
        pthread_attr_setschedparam(&attr, &param);

        int err = pthread_create(&threads[i], &attr, sched_thread, &configs[i]);
        if (err != 0) {
            printf("  [Thread %d] Create failed: %s\n", i, strerror(err));
        }
        pthread_attr_destroy(&attr);
    }

    for (int i = 0; i < n; i++) {
        pthread_join(threads[i], NULL);
    }
}

/* ============================================================
 * Demo 2: Priority inversion
 * ============================================================ */
static pthread_mutex_t shared_mutex;
static volatile int work_done = 0;

void *low_priority_func(void *arg) {
    (void)arg;
    printf("  [LOW]  Acquiring lock...\n");
    pthread_mutex_lock(&shared_mutex);
    printf("  [LOW]  Lock acquired. Doing long work...\n");

    volatile long sum = 0;
    for (long i = 0; i < 100000000L; i++) sum += i;

    printf("  [LOW]  Work done, releasing lock.\n");
    pthread_mutex_unlock(&shared_mutex);
    return NULL;
}

void *medium_priority_func(void *arg) {
    (void)arg;
    usleep(10000);  /* Start slightly after low */
    printf("  [MED]  Running (no lock needed)...\n");

    volatile long sum = 0;
    for (long i = 0; i < 100000000L; i++) sum += i;

    printf("  [MED]  Done.\n");
    return NULL;
}

void *high_priority_func(void *arg) {
    (void)arg;
    usleep(20000);  /* Start after low has the lock */
    printf("  [HIGH] Trying to acquire lock...\n");

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pthread_mutex_lock(&shared_mutex);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double wait_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                     (end.tv_nsec - start.tv_nsec) / 1000000.0;

    printf("  [HIGH] Lock acquired after %.1f ms wait!\n", wait_ms);
    pthread_mutex_unlock(&shared_mutex);
    return NULL;
}

void demo_priority_inversion(int use_inherit) {
    printf("\n--- Demo 2: Priority Inversion %s ---\n",
           use_inherit ? "(with PRIO_INHERIT fix)" : "(no fix)");

    if (geteuid() != 0) {
        printf("  [SKIP] Requires root.\n");
        return;
    }

    /* Setup mutex (optionally with priority inheritance) */
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    if (use_inherit) {
        pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_INHERIT);
    }
    pthread_mutex_init(&shared_mutex, &mattr);
    pthread_mutexattr_destroy(&mattr);

    /* Create threads with different priorities */
    pthread_t low, med, high;
    pthread_attr_t attr;
    struct sched_param param;

    /* Low priority thread */
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    param.sched_priority = 10;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&low, &attr, low_priority_func, NULL);

    /* Medium priority thread */
    param.sched_priority = 50;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&med, &attr, medium_priority_func, NULL);

    /* High priority thread */
    param.sched_priority = 90;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&high, &attr, high_priority_func, NULL);

    pthread_attr_destroy(&attr);

    pthread_join(high, NULL);
    pthread_join(med, NULL);
    pthread_join(low, NULL);

    pthread_mutex_destroy(&shared_mutex);
}

int main(void) {
    printf("=== Thread Scheduling Demo ===\n\n");

    demo_thread_policies();
    demo_priority_inversion(0);  /* Without fix */
    demo_priority_inversion(1);  /* With priority inheritance */

    printf("\n=== Demo Complete ===\n");
    return 0;
}
