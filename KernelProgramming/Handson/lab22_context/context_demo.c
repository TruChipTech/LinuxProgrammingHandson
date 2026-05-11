/*
 * context_demo.c — Kernel Execution Context Detection Module
 *
 * Demonstrates different execution contexts:
 * process, softirq (timer), workqueue, tasklet
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/preempt.h>
#include <linux/delay.h>

static struct timer_list my_timer;
static struct work_struct my_work;
static struct tasklet_struct my_tasklet;

static void report_context(const char *label)
{
    pr_info("[context] %-16s: PID=%-5d comm=%-16s CPU=%d "
            "in_interrupt=%d in_softirq=%d in_hardirq=%d "
            "in_task=%d preemptible=%d\n",
            label,
            current->pid, current->comm, smp_processor_id(),
            !!in_interrupt(), !!in_softirq(), !!in_hardirq(),
            !!in_task(), preemptible());

    /* Additional info about address space */
    if (current->mm)
        pr_info("[context] %-16s: has mm_struct (user process)\n", label);
    else
        pr_info("[context] %-16s: no mm_struct (kernel thread)\n", label);
}

static void timer_fn(struct timer_list *t)
{
    report_context("TIMER (softirq)");
}

static void work_fn(struct work_struct *work)
{
    report_context("WORKQUEUE");
}

static void tasklet_fn(unsigned long data)
{
    report_context("TASKLET");
}

static int __init context_demo_init(void)
{
    pr_info("=== Execution Context Demo ===\n");

    /* 1. Module init — process context (insmod/modprobe) */
    report_context("MODULE_INIT");

    /* 2. Workqueue — process context (kworker) */
    INIT_WORK(&my_work, work_fn);
    schedule_work(&my_work);

    /* 3. Tasklet — softirq context */
    tasklet_init(&my_tasklet, tasklet_fn, 0);
    tasklet_schedule(&my_tasklet);

    /* 4. Timer — softirq context */
    timer_setup(&my_timer, timer_fn, 0);
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(100));

    return 0;
}

static void __exit context_demo_exit(void)
{
    del_timer_sync(&my_timer);
    tasklet_kill(&my_tasklet);
    cancel_work_sync(&my_work);

    report_context("MODULE_EXIT");
    pr_info("=== Context Demo Complete ===\n");
}

module_init(context_demo_init);
module_exit(context_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Kernel Execution Context Detection");
