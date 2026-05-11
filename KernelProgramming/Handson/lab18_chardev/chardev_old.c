/*
 * chardev_old.c — Character Device (Old Method: register_chrdev)
 *
 * Uses the legacy register_chrdev() API.
 * Requires manual: sudo mknod /dev/oldchardev c 240 0
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "oldchardev"
#define MAJOR_NUM   240
#define BUF_SIZE    1024

static char device_buf[BUF_SIZE];
static int buf_len = 0;
static int open_count = 0;

static int dev_open(struct inode *inode, struct file *file)
{
    open_count++;
    pr_info("[oldchardev] Opened (count=%d)\n", open_count);
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    pr_info("[oldchardev] Closed\n");
    return 0;
}

static ssize_t dev_read(struct file *f, char __user *buf,
                         size_t count, loff_t *off)
{
    int to_copy;

    if (*off >= buf_len)
        return 0;

    to_copy = min_t(int, count, buf_len - *off);

    if (copy_to_user(buf, device_buf + *off, to_copy))
        return -EFAULT;

    *off += to_copy;
    pr_info("[oldchardev] Read %d bytes\n", to_copy);
    return to_copy;
}

static ssize_t dev_write(struct file *f, const char __user *buf,
                          size_t count, loff_t *off)
{
    int to_copy = min_t(int, count, BUF_SIZE - 1);

    if (copy_from_user(device_buf, buf, to_copy))
        return -EFAULT;

    buf_len = to_copy;
    device_buf[buf_len] = '\0';
    pr_info("[oldchardev] Wrote %d bytes: \"%s\"\n", to_copy, device_buf);
    return to_copy;
}

static const struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = dev_open,
    .release = dev_release,
    .read    = dev_read,
    .write   = dev_write,
};

static int __init chardev_old_init(void)
{
    int ret = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops);
    if (ret < 0) {
        pr_err("[oldchardev] register_chrdev failed: %d\n", ret);
        return ret;
    }

    pr_info("[oldchardev] Registered with major=%d\n", MAJOR_NUM);
    pr_info("[oldchardev] Create node: sudo mknod /dev/%s c %d 0\n",
            DEVICE_NAME, MAJOR_NUM);
    pr_info("[oldchardev] Set perms:   sudo chmod 666 /dev/%s\n", DEVICE_NAME);
    return 0;
}

static void __exit chardev_old_exit(void)
{
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    pr_info("[oldchardev] Unregistered\n");
}

module_init(chardev_old_init);
module_exit(chardev_old_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Character Device (Old Method)");
