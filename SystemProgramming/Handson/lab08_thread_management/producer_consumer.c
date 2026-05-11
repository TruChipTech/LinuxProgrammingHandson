/*
 * producer_consumer.c — Classic Producer-Consumer with condition variables
 *
 * Build: gcc -Wall -g producer_consumer.c -o producer_consumer -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 8
#define NUM_ITEMS   20

/* Shared circular buffer */
typedef struct {
    int buffer[BUFFER_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t  not_full;
    pthread_cond_t  not_empty;
} bounded_buffer_t;

static bounded_buffer_t bb = {
    .head = 0, .tail = 0, .count = 0,
    .mutex     = PTHREAD_MUTEX_INITIALIZER,
    .not_full  = PTHREAD_COND_INITIALIZER,
    .not_empty = PTHREAD_COND_INITIALIZER,
};

/* ============================================================
 * Producer
 * ============================================================ */
void *producer(void *arg) {
    int id = *(int *)arg;

    for (int i = 0; i < NUM_ITEMS; i++) {
        int item = id * 1000 + i;

        pthread_mutex_lock(&bb.mutex);

        /* Wait while buffer is full */
        while (bb.count == BUFFER_SIZE) {
            printf("  [P%d] Buffer full, waiting...\n", id);
            pthread_cond_wait(&bb.not_full, &bb.mutex);
        }

        /* Add item */
        bb.buffer[bb.head] = item;
        bb.head = (bb.head + 1) % BUFFER_SIZE;
        bb.count++;
        printf("  [P%d] Produced %d  (buffer: %d/%d)\n",
               id, item, bb.count, BUFFER_SIZE);

        /* Signal consumers */
        pthread_cond_signal(&bb.not_empty);
        pthread_mutex_unlock(&bb.mutex);

        usleep(50000 + rand() % 100000);
    }

    printf("  [P%d] Done producing.\n", id);
    return NULL;
}

/* ============================================================
 * Consumer
 * ============================================================ */
void *consumer(void *arg) {
    int id = *(int *)arg;
    int consumed = 0;

    while (1) {
        pthread_mutex_lock(&bb.mutex);

        /* Wait while buffer is empty */
        while (bb.count == 0) {
            pthread_cond_wait(&bb.not_empty, &bb.mutex);
        }

        /* Remove item */
        int item = bb.buffer[bb.tail];
        bb.tail = (bb.tail + 1) % BUFFER_SIZE;
        bb.count--;
        consumed++;
        printf("  [C%d] Consumed %d  (buffer: %d/%d)\n",
               id, item, bb.count, BUFFER_SIZE);

        /* Signal producers */
        pthread_cond_signal(&bb.not_full);
        pthread_mutex_unlock(&bb.mutex);

        usleep(80000 + rand() % 120000);

        /* Exit after consuming enough (simplified) */
        if (consumed >= NUM_ITEMS) break;
    }

    printf("  [C%d] Done consuming (%d items).\n", id, consumed);
    return NULL;
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
    printf("=== Producer-Consumer Demo ===\n");
    printf("Buffer size: %d, Items per producer: %d\n\n", BUFFER_SIZE, NUM_ITEMS);

    srand(42);

    int num_producers = 2;
    int num_consumers = 2;

    pthread_t producers[2], consumers[2];
    int pids[2] = {0, 1};
    int cids[2] = {0, 1};

    /* Start consumers first */
    for (int i = 0; i < num_consumers; i++) {
        pthread_create(&consumers[i], NULL, consumer, &cids[i]);
    }

    /* Start producers */
    for (int i = 0; i < num_producers; i++) {
        pthread_create(&producers[i], NULL, producer, &pids[i]);
    }

    /* Wait for producers */
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producers[i], NULL);
    }

    /* Give consumers time to finish, then cancel */
    sleep(2);
    for (int i = 0; i < num_consumers; i++) {
        pthread_cancel(consumers[i]);
        pthread_join(consumers[i], NULL);
    }

    printf("\n=== Demo Complete ===\n");
    return 0;
}
