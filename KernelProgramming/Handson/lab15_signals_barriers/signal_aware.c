/*
 * signal_aware.c — Signal-Aware Kernel Module with Char Device
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/sched/signal.h>
#include <linux/delay.h>

static DECLARE_WAIT_QUEUE_HEAD(my_wq);
static int data_available = 0;

/*
 * Read blocks until data_available — but is interruptible by signals.
 * Returns -ERESTARTSYS if a signal is pending, which the kernel
 * translates into -EINTR for userspace.
 */
static ssize_t sig_read(struct file *f, char __user *buf,
                         size_t count, loff_t *off)
{
    int ret;

    pr_info("[signal] read() called — blocking until data or signal\n");

    ret = wait_event_interruptible(my_wq, data_available != 0);
    if (ret == -ERESTARTSYS) {
        pr_info("[signal] Interrupted by signal! (signal_pending=%d)\n",
                signal_pending(current));
        return -ERESTARTSYS;
    }

    data_available = 0;
    pr_info("[signal] Data consumed\n");

    if (count > 5) count = 5;
    if (copy_to_user(buf, "hello", count))
        return -EFAULT;

    return count;
}

/* Write wakes up blocked readers */
static ssize_t sig_write(struct file *f, const char __user *buf,
                          size_t count, loff_t *off)
{
    data_available = 1;
    wake_up_interruptible(&my_wq);
    pr_info("[signal] write() — waking readers\n");
    return count;
}

static const struct file_operations sig_fops = {
    .owner = THIS_MODULE,
    .read  = sig_read,
    .write = sig_write,
};

static struct miscdevice sig_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "sigdemo",
    .fops  = &sig_fops,
};

static int __init signal_aware_init(void)
{
    int ret = misc_register(&sig_misc);
    if (ret) {
        pr_err("[signal] Failed to register misc device\n");
        return ret;
    }
    pr_info("[signal] /dev/sigdemo created — try: cat /dev/sigdemo (then Ctrl+C)\n");
    return 0;
}

static void __exit signal_aware_exit(void)
{
    misc_deregister(&sig_misc);
    pr_info("[signal] Module unloaded\n");
}

module_init(signal_aware_init);
module_exit(signal_aware_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Signal-Aware Kernel Module");
