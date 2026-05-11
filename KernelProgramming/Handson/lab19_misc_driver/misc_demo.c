/*
 * misc_demo.c — Miscellaneous Device Driver Demo
 *
 * Creates /dev/mymisc with read/write support.
 * Compare this with lab18's full chardev — much simpler!
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define BUF_SIZE 1024

static char device_buf[BUF_SIZE];
static int buf_len;
static DEFINE_MUTEX(buf_mutex);

static int misc_open(struct inode *inode, struct file *file)
{
    pr_info("[mymisc] Opened by %s (PID %d)\n", current->comm, current->pid);
    return 0;
}

static int misc_release(struct inode *inode, struct file *file)
{
    pr_info("[mymisc] Closed\n");
    return 0;
}

static ssize_t misc_read(struct file *f, char __user *buf,
                          size_t count, loff_t *off)
{
    int to_copy;

    mutex_lock(&buf_mutex);
    if (*off >= buf_len) {
        mutex_unlock(&buf_mutex);
        return 0;
    }

    to_copy = min_t(int, count, buf_len - *off);
    if (copy_to_user(buf, device_buf + *off, to_copy)) {
        mutex_unlock(&buf_mutex);
        return -EFAULT;
    }

    *off += to_copy;
    mutex_unlock(&buf_mutex);
    return to_copy;
}

static ssize_t misc_write(struct file *f, const char __user *buf,
                           size_t count, loff_t *off)
{
    int to_copy = min_t(int, count, BUF_SIZE - 1);

    mutex_lock(&buf_mutex);
    if (copy_from_user(device_buf, buf, to_copy)) {
        mutex_unlock(&buf_mutex);
        return -EFAULT;
    }

    buf_len = to_copy;
    device_buf[buf_len] = '\0';
    mutex_unlock(&buf_mutex);

    pr_info("[mymisc] Wrote %d bytes\n", to_copy);
    return to_copy;
}

static const struct file_operations misc_fops = {
    .owner   = THIS_MODULE,
    .open    = misc_open,
    .release = misc_release,
    .read    = misc_read,
    .write   = misc_write,
};

static struct miscdevice my_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "mymisc",
    .fops  = &misc_fops,
};

static int __init misc_demo_init(void)
{
    int ret = misc_register(&my_misc);
    if (ret) {
        pr_err("[mymisc] Failed to register misc device\n");
        return ret;
    }
    pr_info("[mymisc] Registered /dev/mymisc (minor=%d)\n", my_misc.minor);
    pr_info("[mymisc] Test: echo hello > /dev/mymisc ; cat /dev/mymisc\n");
    return 0;
}

static void __exit misc_demo_exit(void)
{
    misc_deregister(&my_misc);
    pr_info("[mymisc] Unregistered\n");
}

module_init(misc_demo_init);
module_exit(misc_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Misc Device Driver Demo");
