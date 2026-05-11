/*
 * platform_device_manual.c — Manual Platform Device (no DT needed)
 *
 * Load this module BEFORE platform_demo.ko to trigger probe.
 * This is useful for testing on x86 VMs without Device Tree.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>

static struct platform_device *my_pdev;

static int __init manual_pdev_init(void)
{
    my_pdev = platform_device_alloc("my_platform_driver", -1);
    if (!my_pdev)
        return -ENOMEM;

    platform_device_add(my_pdev);
    pr_info("[manual_pdev] Platform device registered\n");
    return 0;
}

static void __exit manual_pdev_exit(void)
{
    platform_device_unregister(my_pdev);
    pr_info("[manual_pdev] Platform device unregistered\n");
}

module_init(manual_pdev_init);
module_exit(manual_pdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Manual Platform Device (for testing without DT)");
