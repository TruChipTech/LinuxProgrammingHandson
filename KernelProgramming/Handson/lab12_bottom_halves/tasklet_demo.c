/*
 * tasklet_demo.c — Tasklet (Bottom Half) Demo Module
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

static unsigned long tasklet_count = 0;

static void my_tasklet_fn(unsigned long data)
{
    tasklet_count++;
    pr_info("[tasklet] Executed #%lu on CPU %d (jiffies=%lu)\n",
            tasklet_count, smp_processor_id(), jiffies);
}

static DECLARE_TASKLET(my_tasklet, my_tasklet_fn, 0);

/* /proc/tasklet_trigger — write to trigger tasklet */
static ssize_t trigger_write(struct file *f, const char __user *buf,
                             size_t count, loff_t *off)
{
    pr_info("[tasklet] Scheduling tasklet from process context (CPU %d)\n",
            smp_processor_id());
    tasklet_schedule(&my_tasklet);
    return count;
}

static const struct proc_ops trigger_ops = {
    .proc_write = trigger_write,
};

static int __init tasklet_demo_init(void)
{
    proc_create("tasklet_trigger", 0222, NULL, &trigger_ops);
    pr_info("[tasklet_demo] Module loaded — write to /proc/tasklet_trigger\n");

    /* Schedule once to demo */
    tasklet_schedule(&my_tasklet);
    return 0;
}

static void __exit tasklet_demo_exit(void)
{
    tasklet_kill(&my_tasklet);
    remove_proc_entry("tasklet_trigger", NULL);
    pr_info("[tasklet_demo] Module unloaded (total: %lu)\n", tasklet_count);
}

module_init(tasklet_demo_init);
module_exit(tasklet_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Tasklet (Bottom Half) Demo");
