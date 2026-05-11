/*
 * workqueue_demo.c — Workqueue Demo Module
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/slab.h>

static struct workqueue_struct *my_wq;
static unsigned long work_count = 0;

/* Regular work */
static void my_work_fn(struct work_struct *work)
{
    work_count++;
    pr_info("[workqueue] Work #%lu executing (PID=%d, CPU=%d, can_sleep=YES)\n",
            work_count, current->pid, smp_processor_id());

    /* Workqueues run in process context — we CAN sleep */
    msleep(100);
    pr_info("[workqueue] Work #%lu completed after 100ms sleep\n", work_count);
}
static DECLARE_WORK(my_work, my_work_fn);

/* Delayed work */
static void my_delayed_fn(struct work_struct *work)
{
    pr_info("[workqueue] Delayed work executed (PID=%d)\n", current->pid);
}
static DECLARE_DELAYED_WORK(my_delayed_work, my_delayed_fn);

/* Dynamic work (allocated) */
struct my_dynamic_work {
    struct work_struct work;
    int data;
};

static void dynamic_work_fn(struct work_struct *work)
{
    struct my_dynamic_work *dw = container_of(work, struct my_dynamic_work, work);
    pr_info("[workqueue] Dynamic work with data=%d\n", dw->data);
    kfree(dw);
}

static int __init workqueue_demo_init(void)
{
    struct my_dynamic_work *dw;

    pr_info("[workqueue] Module loaded\n");

    /* Create a custom workqueue */
    my_wq = create_singlethread_workqueue("my_workqueue");
    if (!my_wq)
        return -ENOMEM;

    /* Schedule regular work */
    queue_work(my_wq, &my_work);

    /* Schedule delayed work (500ms delay) */
    queue_delayed_work(my_wq, &my_delayed_work, msecs_to_jiffies(500));

    /* Schedule on system workqueue */
    schedule_work(&my_work);

    /* Dynamic work */
    dw = kmalloc(sizeof(*dw), GFP_KERNEL);
    if (dw) {
        INIT_WORK(&dw->work, dynamic_work_fn);
        dw->data = 42;
        queue_work(my_wq, &dw->work);
    }

    return 0;
}

static void __exit workqueue_demo_exit(void)
{
    cancel_work_sync(&my_work);
    cancel_delayed_work_sync(&my_delayed_work);
    flush_workqueue(my_wq);
    destroy_workqueue(my_wq);
    pr_info("[workqueue] Module unloaded\n");
}

module_init(workqueue_demo_init);
module_exit(workqueue_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Workqueue Demo");
