/*
 * sync_demo.c — Synchronization Primitives Demo
 *   Spinlock, Mutex, Atomic operations
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>

#define NUM_ITERATIONS 100000

/* --- Unprotected counter (racy!) --- */
static int unprotected = 0;

/* --- Atomic counter --- */
static atomic_t atomic_cnt = ATOMIC_INIT(0);

/* --- Spinlock-protected counter --- */
static int spinlock_cnt = 0;
static DEFINE_SPINLOCK(my_spinlock);

/* --- Mutex-protected counter --- */
static int mutex_cnt = 0;
static DEFINE_MUTEX(my_mutex);

static struct task_struct *threads[2];

static int worker_fn(void *data)
{
    int i;
    int id = (int)(long)data;

    pr_info("[sync_%d] Starting %d iterations\n", id, NUM_ITERATIONS);

    for (i = 0; i < NUM_ITERATIONS; i++) {
        unsigned long flags;

        /* Unprotected (data race!) */
        unprotected++;

        /* Atomic */
        atomic_inc(&atomic_cnt);

        /* Spinlock */
        spin_lock_irqsave(&my_spinlock, flags);
        spinlock_cnt++;
        spin_unlock_irqrestore(&my_spinlock, flags);

        /* Mutex (only in process context, can sleep) */
        mutex_lock(&my_mutex);
        mutex_cnt++;
        mutex_unlock(&my_mutex);
    }

    pr_info("[sync_%d] Done\n", id);
    return 0;
}

static int __init sync_demo_init(void)
{
    int i;

    pr_info("=== Synchronization Demo (expect: %d per counter) ===\n",
            NUM_ITERATIONS * 2);

    for (i = 0; i < 2; i++) {
        threads[i] = kthread_run(worker_fn, (void *)(long)i,
                                 "sync_worker_%d", i);
        if (IS_ERR(threads[i])) {
            pr_err("Failed to create thread %d\n", i);
            threads[i] = NULL;
        }
    }

    /* Wait for threads to finish */
    for (i = 0; i < 2; i++) {
        if (threads[i])
            kthread_stop(threads[i]);
    }

    pr_info("Results (expected %d):\n", NUM_ITERATIONS * 2);
    pr_info("  Unprotected : %d %s\n", unprotected,
            unprotected == NUM_ITERATIONS * 2 ? "(lucky!)" : "(DATA RACE!)");
    pr_info("  Atomic      : %d %s\n", atomic_read(&atomic_cnt),
            atomic_read(&atomic_cnt) == NUM_ITERATIONS * 2 ? "(OK)" : "(BUG!)");
    pr_info("  Spinlock    : %d %s\n", spinlock_cnt,
            spinlock_cnt == NUM_ITERATIONS * 2 ? "(OK)" : "(BUG!)");
    pr_info("  Mutex       : %d %s\n", mutex_cnt,
            mutex_cnt == NUM_ITERATIONS * 2 ? "(OK)" : "(BUG!)");

    return 0;
}

static void __exit sync_demo_exit(void)
{
    pr_info("sync_demo unloaded\n");
}

module_init(sync_demo_init);
module_exit(sync_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Synchronization Primitives Demo");
