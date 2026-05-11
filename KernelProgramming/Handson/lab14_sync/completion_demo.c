/*
 * completion_demo.c — Completion Variable Demo
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/delay.h>

static DECLARE_COMPLETION(data_ready);
static int shared_result = 0;
static struct task_struct *producer;
static struct task_struct *consumer;

static int producer_fn(void *data)
{
    pr_info("[producer] Working on computation...\n");
    msleep(2000);  /* Simulate work */

    shared_result = 42;
    pr_info("[producer] Result ready: %d — signaling consumer\n", shared_result);
    complete(&data_ready);

    /* Wait for stop signal */
    while (!kthread_should_stop())
        msleep(100);
    return 0;
}

static int consumer_fn(void *data)
{
    unsigned long timeout;

    pr_info("[consumer] Waiting for data (timeout 5s)...\n");

    timeout = wait_for_completion_timeout(&data_ready, msecs_to_jiffies(5000));
    if (timeout == 0) {
        pr_err("[consumer] Timed out waiting for data!\n");
    } else {
        pr_info("[consumer] Got result: %d (remaining jiffies: %lu)\n",
                shared_result, timeout);
    }

    /* Wait for stop signal */
    while (!kthread_should_stop())
        msleep(100);
    return 0;
}

static int __init completion_demo_init(void)
{
    pr_info("[completion_demo] Module loaded\n");

    consumer = kthread_run(consumer_fn, NULL, "comp_consumer");
    if (IS_ERR(consumer))
        return PTR_ERR(consumer);

    producer = kthread_run(producer_fn, NULL, "comp_producer");
    if (IS_ERR(producer)) {
        kthread_stop(consumer);
        return PTR_ERR(producer);
    }

    return 0;
}

static void __exit completion_demo_exit(void)
{
    kthread_stop(producer);
    kthread_stop(consumer);
    pr_info("[completion_demo] Module unloaded\n");
}

module_init(completion_demo_init);
module_exit(completion_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Completion Variable Demo");
