/*
 * kfifo_demo.c — Kernel FIFO (Ring Buffer) Demo
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kfifo.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

/* Static FIFO for integers */
static DEFINE_KFIFO(my_fifo, int, 64);  /* 64 elements */

/* /proc/kfifo_write — write int values */
static ssize_t fifo_write(struct file *f, const char __user *buf,
                           size_t count, loff_t *off)
{
    char kbuf[32];
    int val, ret;

    if (count >= sizeof(kbuf))
        return -EINVAL;

    if (copy_from_user(kbuf, buf, count))
        return -EFAULT;
    kbuf[count] = '\0';

    ret = kstrtoint(kbuf, 10, &val);
    if (ret)
        return ret;

    if (kfifo_is_full(&my_fifo)) {
        pr_info("[kfifo] FIFO full!\n");
        return -ENOSPC;
    }

    kfifo_put(&my_fifo, val);
    pr_info("[kfifo] Put: %d (len=%u/%u)\n",
            val, kfifo_len(&my_fifo), kfifo_size(&my_fifo));

    return count;
}

/* /proc/kfifo_read — read one int from FIFO */
static ssize_t fifo_read(struct file *f, char __user *buf,
                          size_t count, loff_t *off)
{
    int val;
    char kbuf[32];
    int len;

    if (*off > 0)
        return 0;

    if (kfifo_is_empty(&my_fifo)) {
        pr_info("[kfifo] FIFO empty!\n");
        return 0;
    }

    if (!kfifo_get(&my_fifo, &val))
        return 0;

    len = snprintf(kbuf, sizeof(kbuf), "%d\n", val);
    if (len > count)
        len = count;

    if (copy_to_user(buf, kbuf, len))
        return -EFAULT;

    *off += len;
    pr_info("[kfifo] Get: %d (remaining=%u)\n", val, kfifo_len(&my_fifo));
    return len;
}

static const struct proc_ops fifo_fops = {
    .proc_read  = fifo_read,
    .proc_write = fifo_write,
};

static int __init kfifo_demo_init(void)
{
    int i, val;

    pr_info("=== kfifo Demo ===\n");
    pr_info("FIFO size: %u elements\n", kfifo_size(&my_fifo));

    /* Pre-fill some values */
    for (i = 1; i <= 5; i++)
        kfifo_put(&my_fifo, i * 10);

    pr_info("Pre-filled 5 values (10,20,30,40,50)\n");
    pr_info("Length: %u, Available: %u\n",
            kfifo_len(&my_fifo), kfifo_avail(&my_fifo));

    /* Peek (non-destructive) */
    if (kfifo_peek(&my_fifo, &val))
        pr_info("Peek: %d (not removed)\n", val);

    /* Get two values */
    if (kfifo_get(&my_fifo, &val))
        pr_info("Get: %d\n", val);
    if (kfifo_get(&my_fifo, &val))
        pr_info("Get: %d\n", val);

    pr_info("After 2 gets: length=%u\n", kfifo_len(&my_fifo));

    /* Create /proc interface */
    proc_create("kfifo_demo", 0666, NULL, &fifo_fops);
    pr_info("Created /proc/kfifo_demo — echo N > /proc/kfifo_demo to push\n");
    pr_info("                           cat /proc/kfifo_demo to pop\n");

    return 0;
}

static void __exit kfifo_demo_exit(void)
{
    remove_proc_entry("kfifo_demo", NULL);
    pr_info("kfifo_demo unloaded (remaining: %u elements)\n",
            kfifo_len(&my_fifo));
}

module_init(kfifo_demo_init);
module_exit(kfifo_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Kernel FIFO (Ring Buffer) Demo");
