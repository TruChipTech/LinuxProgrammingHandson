/*
 * rcu_demo.c — Read-Copy-Update (RCU) Demo
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/delay.h>

struct my_data {
    int value;
    char info[32];
    struct rcu_head rcu;
};

static struct my_data __rcu *global_data;
static struct task_struct *reader_thread;
static struct task_struct *writer_thread;

/* RCU callback — called after grace period */
static void my_rcu_free(struct rcu_head *head)
{
    struct my_data *old = container_of(head, struct my_data, rcu);
    pr_info("[rcu] Freed old data (value=%d)\n", old->value);
    kfree(old);
}

/* Reader — lock-free reads */
static int reader_fn(void *data)
{
    while (!kthread_should_stop()) {
        struct my_data *d;

        rcu_read_lock();
        d = rcu_dereference(global_data);
        if (d)
            pr_info("[rcu-reader] value=%d info=%s\n", d->value, d->info);
        rcu_read_unlock();

        msleep(200);
    }
    return 0;
}

/* Writer — creates new versions */
static int writer_fn(void *data)
{
    int version = 0;

    while (!kthread_should_stop()) {
        struct my_data *new_data, *old_data;

        version++;

        new_data = kmalloc(sizeof(*new_data), GFP_KERNEL);
        if (!new_data) {
            msleep(500);
            continue;
        }

        new_data->value = version;
        snprintf(new_data->info, sizeof(new_data->info), "version_%d", version);

        old_data = rcu_dereference_protected(global_data,
                        lockdep_is_held(&global_data));

        rcu_assign_pointer(global_data, new_data);

        if (old_data)
            call_rcu(&old_data->rcu, my_rcu_free);

        pr_info("[rcu-writer] Published version %d\n", version);
        msleep(1000);
    }
    return 0;
}

static int __init rcu_demo_init(void)
{
    struct my_data *initial;

    pr_info("[rcu_demo] Module loaded\n");

    initial = kmalloc(sizeof(*initial), GFP_KERNEL);
    if (!initial)
        return -ENOMEM;
    initial->value = 0;
    snprintf(initial->info, sizeof(initial->info), "initial");
    rcu_assign_pointer(global_data, initial);

    reader_thread = kthread_run(reader_fn, NULL, "rcu_reader");
    if (IS_ERR(reader_thread)) {
        kfree(initial);
        return PTR_ERR(reader_thread);
    }

    writer_thread = kthread_run(writer_fn, NULL, "rcu_writer");
    if (IS_ERR(writer_thread)) {
        kthread_stop(reader_thread);
        kfree(initial);
        return PTR_ERR(writer_thread);
    }

    return 0;
}

static void __exit rcu_demo_exit(void)
{
    struct my_data *last;

    kthread_stop(writer_thread);
    kthread_stop(reader_thread);

    synchronize_rcu();
    last = rcu_dereference_protected(global_data, 1);
    if (last)
        kfree(last);

    pr_info("[rcu_demo] Module unloaded\n");
}

module_init(rcu_demo_init);
module_exit(rcu_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("RCU Demo");
