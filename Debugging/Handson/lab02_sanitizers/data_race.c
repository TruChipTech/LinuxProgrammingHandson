/*
 * data_race.c — Data race demo for ThreadSanitizer (TSan)
 *
 * Build: gcc -g -fsanitize=thread -fno-omit-frame-pointer -o data_race data_race.c -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS  4
#define ITERATIONS   10000

/* BUG: No synchronization protecting these shared variables */
static int shared_counter = 0;
static int shared_array[100];

void *increment(void *arg)
{
    int id = *(int *)arg;

    for (int i = 0; i < ITERATIONS; i++) {
        /* Data race: concurrent read-modify-write */
        shared_counter++;

        /* Data race: concurrent writes to shared array */
        shared_array[i % 100] = id;
    }

    return NULL;
}

int main(void)
{
    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];

    printf("=== Data Race Demo (TSan) ===\n");
    printf("Starting %d threads...\n\n", NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, increment, &ids[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    printf("Final counter: %d (expected %d)\n",
           shared_counter, NUM_THREADS * ITERATIONS);

    return 0;
}
