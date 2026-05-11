/*
 * shm_demo.c — POSIX Shared Memory with Semaphores
 *
 * Build: gcc -Wall -g shm_demo.c -o shm_demo -lrt -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>

#define SHM_NAME     "/demo_shm"
#define SEM_PROD     "/demo_sem_prod"
#define SEM_CONS     "/demo_sem_cons"
#define SEM_MUTEX    "/demo_sem_mutex"
#define RING_SIZE    8
#define NUM_ITEMS    20

/* Ring buffer stored in shared memory */
typedef struct {
    int buffer[RING_SIZE];
    int head;
    int tail;
    int produced;
    int consumed;
} shared_ring_t;

int main(void) {
    printf("=== Shared Memory + Semaphore Demo ===\n\n");

    /* Clean up previous runs */
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_PROD);
    sem_unlink(SEM_CONS);
    sem_unlink(SEM_MUTEX);

    /* Create shared memory */
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0644);
    if (shm_fd < 0) { perror("shm_open"); return 1; }
    ftruncate(shm_fd, sizeof(shared_ring_t));

    shared_ring_t *ring = mmap(NULL, sizeof(shared_ring_t),
                               PROT_READ | PROT_WRITE, MAP_SHARED,
                               shm_fd, 0);
    if (ring == MAP_FAILED) { perror("mmap"); return 1; }

    memset(ring, 0, sizeof(*ring));

    /* Create semaphores */
    sem_t *sem_prod  = sem_open(SEM_PROD,  O_CREAT, 0644, RING_SIZE); /* slots free */
    sem_t *sem_cons  = sem_open(SEM_CONS,  O_CREAT, 0644, 0);         /* items ready */
    sem_t *sem_mutex = sem_open(SEM_MUTEX, O_CREAT, 0644, 1);         /* mutual excl */

    printf("Shared memory at /dev/shm%s (%zu bytes)\n", SHM_NAME, sizeof(*ring));
    printf("Ring buffer size: %d, producing %d items\n\n", RING_SIZE, NUM_ITEMS);

    pid_t pid = fork();
    if (pid == 0) {
        /* Child: Producer */
        for (int i = 0; i < NUM_ITEMS; i++) {
            sem_wait(sem_prod);   /* Wait for empty slot */
            sem_wait(sem_mutex);  /* Lock */

            ring->buffer[ring->head] = i * 10;
            ring->head = (ring->head + 1) % RING_SIZE;
            ring->produced++;
            printf("  [Producer] Produced %d  (head=%d, total=%d)\n",
                   i * 10, ring->head, ring->produced);

            sem_post(sem_mutex);  /* Unlock */
            sem_post(sem_cons);   /* Signal item ready */
            usleep(30000);
        }
        _exit(0);
    }

    /* Parent: Consumer */
    for (int i = 0; i < NUM_ITEMS; i++) {
        sem_wait(sem_cons);   /* Wait for item */
        sem_wait(sem_mutex);  /* Lock */

        int val = ring->buffer[ring->tail];
        ring->tail = (ring->tail + 1) % RING_SIZE;
        ring->consumed++;
        printf("  [Consumer] Consumed %d  (tail=%d, total=%d)\n",
               val, ring->tail, ring->consumed);

        sem_post(sem_mutex);  /* Unlock */
        sem_post(sem_prod);   /* Signal slot free */
        usleep(50000);
    }

    waitpid(pid, NULL, 0);

    printf("\nFinal: produced=%d, consumed=%d\n", ring->produced, ring->consumed);

    /* Cleanup */
    munmap(ring, sizeof(*ring));
    close(shm_fd);
    shm_unlink(SHM_NAME);
    sem_close(sem_prod);  sem_unlink(SEM_PROD);
    sem_close(sem_cons);  sem_unlink(SEM_CONS);
    sem_close(sem_mutex); sem_unlink(SEM_MUTEX);

    printf("\n=== Demo Complete ===\n");
    return 0;
}
