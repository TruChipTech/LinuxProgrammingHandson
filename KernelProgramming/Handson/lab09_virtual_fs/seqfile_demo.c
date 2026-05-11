/*
 * seqfile_demo.c — Seq_file API Demo Module
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

#define NUM_ITEMS 50

struct my_data {
    int id;
    char name[32];
    unsigned long value;
};

static struct my_data *data_array;

static void *my_seq_start(struct seq_file *s, loff_t *pos)
{
    if (*pos >= NUM_ITEMS)
        return NULL;
    return &data_array[*pos];
}

static void *my_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    (*pos)++;
    if (*pos >= NUM_ITEMS)
        return NULL;
    return &data_array[*pos];
}

static void my_seq_stop(struct seq_file *s, void *v)
{
    /* Nothing to clean up */
}

static int my_seq_show(struct seq_file *s, void *v)
{
    struct my_data *item = v;
    seq_printf(s, "[%3d] %-20s %lu\n", item->id, item->name, item->value);
    return 0;
}

static const struct seq_operations my_seq_ops = {
    .start = my_seq_start,
    .next  = my_seq_next,
    .stop  = my_seq_stop,
    .show  = my_seq_show,
};

static int my_proc_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &my_seq_ops);
}

static const struct proc_ops my_proc_ops = {
    .proc_open    = my_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = seq_release,
};

static int __init seqfile_demo_init(void)
{
    int i;

    data_array = kcalloc(NUM_ITEMS, sizeof(struct my_data), GFP_KERNEL);
    if (!data_array)
        return -ENOMEM;

    for (i = 0; i < NUM_ITEMS; i++) {
        data_array[i].id = i;
        snprintf(data_array[i].name, sizeof(data_array[i].name),
                 "item_%03d", i);
        data_array[i].value = jiffies + i * 100;
    }

    proc_create("seqfile_demo", 0444, NULL, &my_proc_ops);
    pr_info("[seqfile_demo] Module loaded — /proc/seqfile_demo created\n");
    return 0;
}

static void __exit seqfile_demo_exit(void)
{
    remove_proc_entry("seqfile_demo", NULL);
    kfree(data_array);
    pr_info("[seqfile_demo] Module unloaded\n");
}

module_init(seqfile_demo_init);
module_exit(seqfile_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Seq_file API Demo");
