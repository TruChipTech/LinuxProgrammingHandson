/*
 * kthread_multi.c — Multiple Kernel Threads with Shared Data
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/moduleparam.h>

static int num_threads = 2;
module_param(num_threads, int, 0444);
MODULE_PARM_DESC(num_threads, "Number of worker threads (default 2, max 8)");

#define MAX_THREADS 8

static struct task_struct *threads[MAX_THREADS];
static spinlock_t counter_lock;
static int shared_counter = 0;

static int worker_fn(void *data)
{
    int id = (int)(long)data;

    pr_info("[worker_%d] Started on CPU %d\n", id, smp_processor_id());

    while (!kthread_should_stop()) {
        unsigned long flags;

        spin_lock_irqsave(&counter_lock, flags);
        shared_counter++;
        pr_info("[worker_%d] counter=%d (CPU %d)\n",
                id, shared_counter, smp_processor_id());
        spin_unlock_irqrestore(&counter_lock, flags);

        msleep(500 + id * 100);  /* Stagger threads */
    }

    pr_info("[worker_%d] Stopping\n", id);
    return 0;
}

static int __init kthread_multi_init(void)
{
    int i;

    if (num_threads > MAX_THREADS)
        num_threads = MAX_THREADS;

    spin_lock_init(&counter_lock);

    pr_info("[kthread_multi] Creating %d threads\n", num_threads);

    for (i = 0; i < num_threads; i++) {
        threads[i] = kthread_run(worker_fn, (void *)(long)i,
                                 "worker_thread_%d", i);
        if (IS_ERR(threads[i])) {
            pr_err("[kthread_multi] Failed to create thread %d\n", i);
            threads[i] = NULL;
        }
    }

    return 0;
}

static void __exit kthread_multi_exit(void)
{
    int i;

    for (i = 0; i < num_threads; i++) {
        if (threads[i])
            kthread_stop(threads[i]);
    }

    pr_info("[kthread_multi] All threads stopped (counter=%d)\n", shared_counter);
}

module_init(kthread_multi_init);
module_exit(kthread_multi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Multiple Kernel Threads Demo");
