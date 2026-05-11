/*
 * hello_module.c — Hello World Kernel Module
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

static int __init hello_init(void)
{
    pr_info("Hello, Kernel World! Module loaded.\n");
    pr_info("  Module: %s\n", THIS_MODULE->name);
    pr_info("  PID of insmod: %d (%s)\n", current->pid, current->comm);
    return 0;
}

static void __exit hello_exit(void)
{
    pr_info("Goodbye, Kernel World! Module unloaded.\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Hello World Kernel Module");
MODULE_VERSION("1.0");
