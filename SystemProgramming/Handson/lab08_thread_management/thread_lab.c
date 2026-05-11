/*
 * thread_lab.c — POSIX Threads Basics
 *
 * Demonstrates pthread_create, join, mutexes, and thread-local storage.
 *
 * Build: gcc -Wall -g thread_lab.c -o thread_lab -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 4

/* ============================================================
 * Demo 1: Basic thread creation and joining
 * ============================================================ */
void *basic_thread_func(void *arg) {
    int id = *(int *)arg;
    printf("  [Thread %d] TID=%lu starting\n", id, pthread_self());
    usleep(100000 * (id + 1));  /* Stagger */
    printf("  [Thread %d] done\n", id);
    return (void *)(long)(id * 10);  /* Return a value */
}

void demo_basic_threads(void) {
    printf("--- Demo 1: Basic Threads ---\n");

    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];

    /* Create threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        if (pthread_create(&threads[i], NULL, basic_thread_func, &ids[i]) != 0) {
            perror("pthread_create");
        }
    }

    /* Join and get return values */
    for (int i = 0; i < NUM_THREADS; i++) {
        void *ret;
        pthread_join(threads[i], &ret);
        printf("  [Main] Thread %d returned %ld\n", i, (long)ret);
    }
}

/* ============================================================
 * Demo 2: Race condition without mutex
 * ============================================================ */
static int shared_counter = 0;

void *unsafe_increment(void *arg) {
    int iterations = *(int *)arg;
    for (int i = 0; i < iterations; i++) {
        shared_counter++;  /* RACE CONDITION! */
    }
    return NULL;
}

void demo_race_condition(void) {
    printf("\n--- Demo 2: Race Condition (No Mutex) ---\n");

    int iterations = 1000000;
    shared_counter = 0;

    pthread_t t1, t2;
    pthread_create(&t1, NULL, unsafe_increment, &iterations);
    pthread_create(&t2, NULL, unsafe_increment, &iterations);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("  Expected: %d\n", iterations * 2);
    printf("  Actual:   %d\n", shared_counter);
    printf("  Lost:     %d updates (race condition!)\n",
           iterations * 2 - shared_counter);
}

/* ============================================================
 * Demo 3: Fix with mutex
 * ============================================================ */
static pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
static int safe_counter = 0;

void *safe_increment(void *arg) {
    int iterations = *(int *)arg;
    for (int i = 0; i < iterations; i++) {
        pthread_mutex_lock(&counter_mutex);
        safe_counter++;
        pthread_mutex_unlock(&counter_mutex);
    }
    return NULL;
}

void demo_mutex_fix(void) {
    printf("\n--- Demo 3: Fixed with Mutex ---\n");

    int iterations = 1000000;
    safe_counter = 0;

    pthread_t t1, t2;
    pthread_create(&t1, NULL, safe_increment, &iterations);
    pthread_create(&t2, NULL, safe_increment, &iterations);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("  Expected: %d\n", iterations * 2);
    printf("  Actual:   %d\n", safe_counter);
    printf("  Result:   %s\n",
           safe_counter == iterations * 2 ? "CORRECT" : "WRONG");
}

/* ============================================================
 * Demo 4: Thread-local storage
 * ============================================================ */
static __thread int tls_value = 0;  /* Thread-local variable */

void *tls_thread_func(void *arg) {
    int id = *(int *)arg;
    tls_value = id * 100;
    printf("  [Thread %d] TLS value = %d\n", id, tls_value);
    usleep(50000);
    printf("  [Thread %d] TLS still = %d (unchanged by others)\n",
           id, tls_value);
    return NULL;
}

void demo_tls(void) {
    printf("\n--- Demo 4: Thread-Local Storage ---\n");

    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, tls_thread_func, &ids[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
}

/* ============================================================
 * Demo 5: Thread attributes (detached, stack size)
 * ============================================================ */
void *detached_func(void *arg) {
    (void)arg;
    printf("  [Detached] Running... (cannot be joined)\n");
    return NULL;
}

void demo_attributes(void) {
    printf("\n--- Demo 5: Thread Attributes ---\n");

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    /* Set detached state */
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    /* Set custom stack size */
    size_t stack_size;
    pthread_attr_getstacksize(&attr, &stack_size);
    printf("  Default stack size: %zu bytes (%zu KB)\n",
           stack_size, stack_size / 1024);

    pthread_attr_setstacksize(&attr, 2 * 1024 * 1024);  /* 2 MB */
    pthread_attr_getstacksize(&attr, &stack_size);
    printf("  Custom stack size:  %zu bytes (%zu KB)\n",
           stack_size, stack_size / 1024);

    pthread_t t;
    pthread_create(&t, &attr, detached_func, NULL);
    /* Cannot join a detached thread */
    usleep(100000);  /* Let it finish */

    pthread_attr_destroy(&attr);
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
    printf("=== POSIX Threads Lab ===\n\n");

    demo_basic_threads();
    demo_race_condition();
    demo_mutex_fix();
    demo_tls();
    demo_attributes();

    printf("\n=== Lab Complete ===\n");
    return 0;
}
