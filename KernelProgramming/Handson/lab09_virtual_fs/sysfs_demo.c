/*
 * sysfs_demo.c — Sysfs Interface Demo Module
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>

static struct kobject *mydevice_kobj;
static int device_value = 0;
static char device_status[64] = "idle";

/* /sys/kernel/mydevice/value */
static ssize_t value_show(struct kobject *kobj, struct kobj_attribute *attr,
                          char *buf)
{
    return sysfs_emit(buf, "%d\n", device_value);
}

static ssize_t value_store(struct kobject *kobj, struct kobj_attribute *attr,
                           const char *buf, size_t count)
{
    int ret = kstrtoint(buf, 10, &device_value);
    if (ret)
        return ret;
    pr_info("[sysfs_demo] Value set to %d\n", device_value);
    return count;
}
static struct kobj_attribute value_attr = __ATTR_RW(value);

/* /sys/kernel/mydevice/status */
static ssize_t status_show(struct kobject *kobj, struct kobj_attribute *attr,
                           char *buf)
{
    return sysfs_emit(buf, "%s\n", device_status);
}

static ssize_t status_store(struct kobject *kobj, struct kobj_attribute *attr,
                            const char *buf, size_t count)
{
    if (count >= sizeof(device_status))
        return -EINVAL;
    strncpy(device_status, buf, count);
    device_status[count] = '\0';
    if (count > 0 && device_status[count - 1] == '\n')
        device_status[count - 1] = '\0';
    pr_info("[sysfs_demo] Status set to: %s\n", device_status);
    return count;
}
static struct kobj_attribute status_attr = __ATTR_RW(status);

/* Attribute group */
static struct attribute *mydevice_attrs[] = {
    &value_attr.attr,
    &status_attr.attr,
    NULL,
};
static struct attribute_group mydevice_group = {
    .attrs = mydevice_attrs,
};

static int __init sysfs_demo_init(void)
{
    int ret;

    mydevice_kobj = kobject_create_and_add("mydevice", kernel_kobj);
    if (!mydevice_kobj)
        return -ENOMEM;

    ret = sysfs_create_group(mydevice_kobj, &mydevice_group);
    if (ret) {
        kobject_put(mydevice_kobj);
        return ret;
    }

    pr_info("[sysfs_demo] Module loaded — /sys/kernel/mydevice/ created\n");
    return 0;
}

static void __exit sysfs_demo_exit(void)
{
    sysfs_remove_group(mydevice_kobj, &mydevice_group);
    kobject_put(mydevice_kobj);
    pr_info("[sysfs_demo] Module unloaded\n");
}

module_init(sysfs_demo_init);
module_exit(sysfs_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Sysfs Interface Demo");
