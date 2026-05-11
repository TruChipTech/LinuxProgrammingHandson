/*
 * thread_debug.c — Multi-threaded program with data race
 *
 * Multiple threads increment a shared counter without synchronization.
 * Practice debugging with:
 *   gdb ./thread_debug
 *   (gdb) info threads
 *   (gdb) thread N
 *   (gdb) set scheduler-locking on
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS  4
#define ITERATIONS   100000

/* BUG: No lock protecting this shared counter */
static int shared_counter = 0;

void *worker_thread(void *arg)
{
    int id = *(int *)arg;
    int local_count = 0;

    for (int i = 0; i < ITERATIONS; i++) {
        /* BUG: Data race — read-modify-write without lock */
        int tmp = shared_counter;
        tmp++;
        shared_counter = tmp;
        local_count++;
    }

    printf("Thread %d: did %d increments, counter now = %d\n",
           id, local_count, shared_counter);
    return NULL;
}

int main(void)
{
    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];

    printf("Starting %d threads, each doing %d increments\n",
           NUM_THREADS, ITERATIONS);
    printf("Expected final counter: %d\n\n", NUM_THREADS * ITERATIONS);

    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, worker_thread, &ids[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    printf("\nFinal counter: %d (expected %d) — %s\n",
           shared_counter, NUM_THREADS * ITERATIONS,
           shared_counter == NUM_THREADS * ITERATIONS ? "OK" : "RACE DETECTED!");

    return 0;
}
