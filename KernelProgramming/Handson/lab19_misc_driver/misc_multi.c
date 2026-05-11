/*
 * misc_multi.c — Misc Device with Per-FD Private Data
 *
 * Each open() gets its own independent buffer via file->private_data.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define BUF_SIZE 1024

struct misc_priv {
    char buf[BUF_SIZE];
    int len;
};

static int misc_open(struct inode *inode, struct file *file)
{
    struct misc_priv *priv;

    priv = kzalloc(sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    file->private_data = priv;
    pr_info("[misc_multi] Opened — private buffer allocated\n");
    return 0;
}

static int misc_release(struct inode *inode, struct file *file)
{
    kfree(file->private_data);
    pr_info("[misc_multi] Closed — private buffer freed\n");
    return 0;
}

static ssize_t misc_read(struct file *f, char __user *buf,
                          size_t count, loff_t *off)
{
    struct misc_priv *priv = f->private_data;
    int to_copy;

    if (*off >= priv->len)
        return 0;

    to_copy = min_t(int, count, priv->len - *off);
    if (copy_to_user(buf, priv->buf + *off, to_copy))
        return -EFAULT;

    *off += to_copy;
    return to_copy;
}

static ssize_t misc_write(struct file *f, const char __user *buf,
                           size_t count, loff_t *off)
{
    struct misc_priv *priv = f->private_data;
    int to_copy = min_t(int, count, BUF_SIZE - 1);

    if (copy_from_user(priv->buf, buf, to_copy))
        return -EFAULT;

    priv->len = to_copy;
    priv->buf[priv->len] = '\0';
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
    .name  = "misc_multi",
    .fops  = &misc_fops,
};

static int __init misc_multi_init(void)
{
    int ret = misc_register(&my_misc);
    if (ret) return ret;
    pr_info("[misc_multi] /dev/misc_multi created\n");
    return 0;
}

static void __exit misc_multi_exit(void)
{
    misc_deregister(&my_misc);
    pr_info("[misc_multi] Removed\n");
}

module_init(misc_multi_init);
module_exit(misc_multi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Misc Device with Per-FD Private Data");
