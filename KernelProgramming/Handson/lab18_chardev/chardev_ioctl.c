/*
 * chardev_ioctl.c — Character Device with ioctl
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>

#define DEVICE_NAME "ioctldev"
#define CLASS_NAME  "ioctl_class"
#define BUF_SIZE    4096

/* ioctl command definitions */
#define IOCTL_MAGIC  'k'
#define IOCTL_RESET       _IO(IOCTL_MAGIC, 0)
#define IOCTL_SET_MSG     _IOW(IOCTL_MAGIC, 1, char[BUF_SIZE])
#define IOCTL_GET_MSG     _IOR(IOCTL_MAGIC, 2, char[BUF_SIZE])
#define IOCTL_GET_LEN     _IOR(IOCTL_MAGIC, 3, int)
#define IOCTL_GET_COUNT   _IOR(IOCTL_MAGIC, 4, int)

static dev_t dev_num;
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;

static char device_buf[BUF_SIZE];
static int buf_len = 0;
static int ioctl_count = 0;
static DEFINE_MUTEX(dev_mutex);

static int dev_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
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
    return to_copy;
}

static long dev_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    int ret = 0;

    mutex_lock(&dev_mutex);
    ioctl_count++;

    switch (cmd) {
    case IOCTL_RESET:
        memset(device_buf, 0, BUF_SIZE);
        buf_len = 0;
        pr_info("[ioctldev] Buffer reset\n");
        break;

    case IOCTL_SET_MSG:
        if (copy_from_user(device_buf, (char __user *)arg, BUF_SIZE - 1)) {
            ret = -EFAULT;
            break;
        }
        device_buf[BUF_SIZE - 1] = '\0';
        buf_len = strlen(device_buf);
        pr_info("[ioctldev] Set message: \"%s\" (len=%d)\n", device_buf, buf_len);
        break;

    case IOCTL_GET_MSG:
        if (copy_to_user((char __user *)arg, device_buf, buf_len + 1)) {
            ret = -EFAULT;
            break;
        }
        pr_info("[ioctldev] Get message: \"%s\"\n", device_buf);
        break;

    case IOCTL_GET_LEN:
        if (copy_to_user((int __user *)arg, &buf_len, sizeof(int)))
            ret = -EFAULT;
        break;

    case IOCTL_GET_COUNT:
        if (copy_to_user((int __user *)arg, &ioctl_count, sizeof(int)))
            ret = -EFAULT;
        break;

    default:
        pr_warn("[ioctldev] Unknown ioctl cmd: 0x%x\n", cmd);
        ret = -ENOTTY;
    }

    mutex_unlock(&dev_mutex);
    return ret;
}

static const struct file_operations fops = {
    .owner          = THIS_MODULE,
    .open           = dev_open,
    .release        = dev_release,
    .read           = dev_read,
    .write          = dev_write,
    .unlocked_ioctl = dev_ioctl,
};

static int __init chardev_ioctl_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) return ret;

    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;
    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) goto err_cdev;

    my_class = class_create(CLASS_NAME);
    if (IS_ERR(my_class)) { ret = PTR_ERR(my_class); goto err_class; }

    my_device = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(my_device)) { ret = PTR_ERR(my_device); goto err_device; }

    pr_info("[ioctldev] /dev/%s created (major=%d)\n",
            DEVICE_NAME, MAJOR(dev_num));
    return 0;

err_device:
    class_destroy(my_class);
err_class:
    cdev_del(&my_cdev);
err_cdev:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void __exit chardev_ioctl_exit(void)
{
    device_destroy(my_class, dev_num);
    class_destroy(my_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("[ioctldev] Removed (total ioctls: %d)\n", ioctl_count);
}

module_init(chardev_ioctl_init);
module_exit(chardev_ioctl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Character Device with ioctl");
