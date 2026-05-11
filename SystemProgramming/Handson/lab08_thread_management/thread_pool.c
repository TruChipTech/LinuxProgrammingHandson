/*
 * thread_pool.c — Simple Thread Pool Implementation
 *
 * Build: gcc -Wall -g thread_pool.c -o thread_pool -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

/* ============================================================
 * Task queue
 * ============================================================ */
typedef void (*task_func_t)(void *arg);

typedef struct task {
    task_func_t func;
    void *arg;
    struct task *next;
} task_t;

typedef struct {
    task_t *head;
    task_t *tail;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    int shutdown;
    int task_count;
} task_queue_t;

/* ============================================================
 * Thread Pool
 * ============================================================ */
typedef struct {
    pthread_t *threads;
    int num_threads;
    task_queue_t queue;
    int tasks_completed;
    pthread_mutex_t stats_mutex;
} thread_pool_t;

/* Worker function */
void *worker(void *arg) {
    thread_pool_t *pool = (thread_pool_t *)arg;

    while (1) {
        pthread_mutex_lock(&pool->queue.mutex);

        /* Wait for a task or shutdown */
        while (pool->queue.head == NULL && !pool->queue.shutdown) {
            pthread_cond_wait(&pool->queue.cond, &pool->queue.mutex);
        }

        if (pool->queue.shutdown && pool->queue.head == NULL) {
            pthread_mutex_unlock(&pool->queue.mutex);
            break;
        }

        /* Dequeue task */
        task_t *task = pool->queue.head;
        pool->queue.head = task->next;
        if (pool->queue.head == NULL) {
            pool->queue.tail = NULL;
        }
        pool->queue.task_count--;

        pthread_mutex_unlock(&pool->queue.mutex);

        /* Execute task */
        task->func(task->arg);
        free(task);

        /* Update stats */
        pthread_mutex_lock(&pool->stats_mutex);
        pool->tasks_completed++;
        pthread_mutex_unlock(&pool->stats_mutex);
    }

    return NULL;
}

/* Initialize pool */
thread_pool_t *pool_create(int num_threads) {
    thread_pool_t *pool = calloc(1, sizeof(thread_pool_t));
    pool->num_threads = num_threads;
    pool->threads = calloc(num_threads, sizeof(pthread_t));
    pool->tasks_completed = 0;

    pthread_mutex_init(&pool->queue.mutex, NULL);
    pthread_cond_init(&pool->queue.cond, NULL);
    pthread_mutex_init(&pool->stats_mutex, NULL);
    pool->queue.shutdown = 0;
    pool->queue.task_count = 0;

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&pool->threads[i], NULL, worker, pool);
    }

    return pool;
}

/* Submit task */
void pool_submit(thread_pool_t *pool, task_func_t func, void *arg) {
    task_t *task = malloc(sizeof(task_t));
    task->func = func;
    task->arg = arg;
    task->next = NULL;

    pthread_mutex_lock(&pool->queue.mutex);
    if (pool->queue.tail) {
        pool->queue.tail->next = task;
    } else {
        pool->queue.head = task;
    }
    pool->queue.tail = task;
    pool->queue.task_count++;
    pthread_cond_signal(&pool->queue.cond);
    pthread_mutex_unlock(&pool->queue.mutex);
}

/* Shutdown and destroy pool */
void pool_destroy(thread_pool_t *pool) {
    pthread_mutex_lock(&pool->queue.mutex);
    pool->queue.shutdown = 1;
    pthread_cond_broadcast(&pool->queue.cond);
    pthread_mutex_unlock(&pool->queue.mutex);

    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    /* Cleanup remaining tasks */
    task_t *t = pool->queue.head;
    while (t) {
        task_t *next = t->next;
        free(t);
        t = next;
    }

    pthread_mutex_destroy(&pool->queue.mutex);
    pthread_cond_destroy(&pool->queue.cond);
    pthread_mutex_destroy(&pool->stats_mutex);
    free(pool->threads);
    free(pool);
}

/* ============================================================
 * Example tasks
 * ============================================================ */
void compute_task(void *arg) {
    int id = *(int *)arg;
    printf("  [Worker %lu] Task %d: computing...\n", pthread_self() % 1000, id);

    /* Simulate work */
    volatile long sum = 0;
    for (long i = 0; i < 10000000L; i++) {
        sum += i;
    }

    printf("  [Worker %lu] Task %d: done (sum=%ld)\n",
           pthread_self() % 1000, id, (long)sum);
    free(arg);
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
    printf("=== Thread Pool Demo ===\n\n");

    int num_workers = 4;
    int num_tasks = 12;

    printf("Creating pool with %d workers...\n", num_workers);
    thread_pool_t *pool = pool_create(num_workers);

    printf("Submitting %d tasks...\n\n", num_tasks);
    for (int i = 0; i < num_tasks; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pool_submit(pool, compute_task, id);
    }

    /* Wait a bit for tasks to complete */
    sleep(3);

    pthread_mutex_lock(&pool->stats_mutex);
    printf("\nTasks completed: %d/%d\n", pool->tasks_completed, num_tasks);
    pthread_mutex_unlock(&pool->stats_mutex);

    printf("Shutting down pool...\n");
    pool_destroy(pool);

    printf("\n=== Demo Complete ===\n");
    return 0;
}
