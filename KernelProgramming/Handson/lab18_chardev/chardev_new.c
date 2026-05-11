/*
 * chardev_new.c — Character Device (New Method)
 *
 * Uses alloc_chrdev_region + cdev + class_create + device_create
 * for automatic /dev node creation via udev.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "newchardev"
#define CLASS_NAME  "newchar_class"
#define BUF_SIZE    4096

static dev_t dev_num;
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;

static char device_buf[BUF_SIZE];
static int buf_len = 0;
static DEFINE_MUTEX(dev_mutex);

static int dev_open(struct inode *inode, struct file *file)
{
    pr_info("[newchardev] Opened (major=%d, minor=%d)\n",
            imajor(inode), iminor(inode));
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    pr_info("[newchardev] Closed\n");
    return 0;
}

static ssize_t dev_read(struct file *f, char __user *buf,
                         size_t count, loff_t *off)
{
    int to_copy;

    mutex_lock(&dev_mutex);

    if (*off >= buf_len) {
        mutex_unlock(&dev_mutex);
        return 0;
    }

    to_copy = min_t(int, count, buf_len - *off);

    if (copy_to_user(buf, device_buf + *off, to_copy)) {
        mutex_unlock(&dev_mutex);
        return -EFAULT;
    }

    *off += to_copy;
    mutex_unlock(&dev_mutex);

    pr_info("[newchardev] Read %d bytes\n", to_copy);
    return to_copy;
}

static ssize_t dev_write(struct file *f, const char __user *buf,
                          size_t count, loff_t *off)
{
    int to_copy = min_t(int, count, BUF_SIZE - 1);

    mutex_lock(&dev_mutex);

    if (copy_from_user(device_buf, buf, to_copy)) {
        mutex_unlock(&dev_mutex);
        return -EFAULT;
    }

    buf_len = to_copy;
    device_buf[buf_len] = '\0';
    mutex_unlock(&dev_mutex);

    pr_info("[newchardev] Wrote %d bytes\n", to_copy);
    return to_copy;
}

static const struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = dev_open,
    .release = dev_release,
    .read    = dev_read,
    .write   = dev_write,
};

static int __init chardev_new_init(void)
{
    int ret;

    /* 1. Dynamically allocate major/minor numbers */
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("[newchardev] alloc_chrdev_region failed: %d\n", ret);
        return ret;
    }
    pr_info("[newchardev] Allocated: major=%d, minor=%d\n",
            MAJOR(dev_num), MINOR(dev_num));

    /* 2. Initialize and add cdev */
    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;
    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("[newchardev] cdev_add failed: %d\n", ret);
        goto err_cdev;
    }

    /* 3. Create device class (visible in /sys/class/) */
    my_class = class_create(CLASS_NAME);
    if (IS_ERR(my_class)) {
        pr_err("[newchardev] class_create failed\n");
        ret = PTR_ERR(my_class);
        goto err_class;
    }

    /* 4. Create device — triggers udev to create /dev/newchardev */
    my_device = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(my_device)) {
        pr_err("[newchardev] device_create failed\n");
        ret = PTR_ERR(my_device);
        goto err_device;
    }

    pr_info("[newchardev] /dev/%s created automatically via udev\n", DEVICE_NAME);
    return 0;

err_device:
    class_destroy(my_class);
err_class:
    cdev_del(&my_cdev);
err_cdev:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void __exit chardev_new_exit(void)
{
    device_destroy(my_class, dev_num);
    class_destroy(my_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("[newchardev] Removed\n");
}

module_init(chardev_new_init);
module_exit(chardev_new_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Character Device (New Method)");
