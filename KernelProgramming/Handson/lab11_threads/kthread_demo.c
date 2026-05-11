/*
 * kthread_demo.c — Basic Kernel Thread Demo
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/sched.h>

static struct task_struct *my_thread;
static int counter = 0;

static int thread_fn(void *data)
{
    pr_info("[kthread] Thread started (PID=%d)\n", current->pid);

    while (!kthread_should_stop()) {
        counter++;
        pr_info("[kthread] Counter = %d (on CPU %d)\n",
                counter, smp_processor_id());
        msleep(1000);
    }

    pr_info("[kthread] Thread stopping (counter=%d)\n", counter);
    return 0;
}

static int __init kthread_demo_init(void)
{
    pr_info("[kthread] Module loaded — creating thread\n");

    my_thread = kthread_run(thread_fn, NULL, "my_kthread");
    if (IS_ERR(my_thread)) {
        pr_err("[kthread] Failed to create thread\n");
        return PTR_ERR(my_thread);
    }

    pr_info("[kthread] Thread created: PID=%d\n", my_thread->pid);
    return 0;
}

static void __exit kthread_demo_exit(void)
{
    if (my_thread) {
        kthread_stop(my_thread);
        pr_info("[kthread] Thread stopped\n");
    }
    pr_info("[kthread] Module unloaded\n");
}

module_init(kthread_demo_init);
module_exit(kthread_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Basic Kernel Thread Demo");
