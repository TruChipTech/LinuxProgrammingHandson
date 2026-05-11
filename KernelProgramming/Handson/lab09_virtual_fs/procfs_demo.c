/*
 * procfs_demo.c — Procfs Interface Demo Module
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

static struct proc_dir_entry *proc_dir;
static char config_buf[256] = "default_config";
static unsigned long read_count = 0;
static unsigned long write_count = 0;

/* /proc/mydriver_info — read-only */
static ssize_t info_read(struct file *f, char __user *buf,
                         size_t count, loff_t *off)
{
    char tmp[256];
    int len;

    if (*off > 0)
        return 0;

    len = snprintf(tmp, sizeof(tmp),
                   "Driver: procfs_demo\n"
                   "Version: 1.0\n"
                   "Uptime: %lu seconds\n"
                   "HZ: %d\n",
                   jiffies / HZ, HZ);

    if (len > count) len = count;
    if (copy_to_user(buf, tmp, len))
        return -EFAULT;

    *off += len;
    read_count++;
    return len;
}

static const struct proc_ops info_ops = {
    .proc_read = info_read,
};

/* /proc/mydriver_stats — read-only */
static ssize_t stats_read(struct file *f, char __user *buf,
                          size_t count, loff_t *off)
{
    char tmp[128];
    int len;

    if (*off > 0) return 0;

    len = snprintf(tmp, sizeof(tmp),
                   "Reads:  %lu\nWrites: %lu\n", read_count, write_count);
    if (len > count) len = count;
    if (copy_to_user(buf, tmp, len))
        return -EFAULT;

    *off += len;
    return len;
}

static const struct proc_ops stats_ops = {
    .proc_read = stats_read,
};

/* /proc/mydriver_config — read/write */
static ssize_t config_read(struct file *f, char __user *buf,
                           size_t count, loff_t *off)
{
    int len = strlen(config_buf);
    if (*off >= len) return 0;

    len -= *off;
    if (len > count) len = count;
    if (copy_to_user(buf, config_buf + *off, len))
        return -EFAULT;

    *off += len;
    return len;
}

static ssize_t config_write(struct file *f, const char __user *buf,
                            size_t count, loff_t *off)
{
    if (count >= sizeof(config_buf))
        return -EINVAL;

    if (copy_from_user(config_buf, buf, count))
        return -EFAULT;

    config_buf[count] = '\0';
    /* Remove trailing newline */
    if (count > 0 && config_buf[count - 1] == '\n')
        config_buf[count - 1] = '\0';

    write_count++;
    pr_info("[procfs_demo] Config set to: %s\n", config_buf);
    return count;
}

static const struct proc_ops config_ops = {
    .proc_read  = config_read,
    .proc_write = config_write,
};

static int __init procfs_demo_init(void)
{
    proc_dir = proc_mkdir("mydriver", NULL);
    if (!proc_dir) return -ENOMEM;

    proc_create("mydriver_info", 0444, NULL, &info_ops);
    proc_create("mydriver_stats", 0444, NULL, &stats_ops);
    proc_create("mydriver_config", 0666, NULL, &config_ops);

    pr_info("[procfs_demo] Module loaded — /proc/mydriver_* created\n");
    return 0;
}

static void __exit procfs_demo_exit(void)
{
    remove_proc_entry("mydriver_config", NULL);
    remove_proc_entry("mydriver_stats", NULL);
    remove_proc_entry("mydriver_info", NULL);
    remove_proc_entry("mydriver", NULL);
    pr_info("[procfs_demo] Module unloaded\n");
}

module_init(procfs_demo_init);
module_exit(procfs_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Procfs Interface Demo");
