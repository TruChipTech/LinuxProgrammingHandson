/*
 * printk_demo.c — Kernel Logging (printk) Demo Module
 *
 * Demonstrates all printk log levels, rate limiting, and format specifiers.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/slab.h>

static int __init printk_demo_init(void)
{
    void *ptr;
    unsigned char mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE};

    pr_info("=== printk Demo Module Loaded ===\n");

    /* Log levels */
    pr_emerg("This is KERN_EMERG (level 0)\n");
    pr_alert("This is KERN_ALERT (level 1)\n");
    pr_crit("This is KERN_CRIT (level 2)\n");
    pr_err("This is KERN_ERR (level 3)\n");
    pr_warn("This is KERN_WARNING (level 4)\n");
    pr_notice("This is KERN_NOTICE (level 5)\n");
    pr_info("This is KERN_INFO (level 6)\n");
    pr_debug("This is KERN_DEBUG (level 7) — needs dynamic debug\n");

    /* Old style printk with explicit level */
    printk(KERN_INFO "Old-style printk with KERN_INFO\n");

    /* Format specifiers */
    pr_info("--- Format Specifiers ---\n");
    pr_info("Integer: %d, Unsigned: %u, Hex: 0x%x\n", -42, 42, 0xDEAD);
    pr_info("Long: %ld, Size: %zu\n", (long)jiffies, sizeof(void *));
    pr_info("String: %s\n", "hello kernel");

    /* Pointer printing — kernel hashes pointers for security */
    ptr = kmalloc(64, GFP_KERNEL);
    pr_info("Pointer (hashed):  %p\n", ptr);    /* Hashed for security */
    pr_info("Pointer (kernel):  %pK\n", ptr);   /* Depends on kptr_restrict */
    pr_info("Pointer (phys):    %pa\n", &ptr);   /* Physical address */
    kfree(ptr);

    /* MAC address */
    pr_info("MAC address: %pM\n", mac);
    pr_info("MAC (upper): %pMF\n", mac);

    /* Jiffies and HZ */
    pr_info("HZ = %d, jiffies = %lu\n", HZ, jiffies);

    /* Rate limited printing */
    {
        int i;
        for (i = 0; i < 100; i++) {
            printk_ratelimited(KERN_INFO "Rate limited msg #%d\n", i);
        }
        pr_info("Sent 100 rate-limited messages (many suppressed)\n");
    }

    return 0;
}

static void __exit printk_demo_exit(void)
{
    pr_info("=== printk Demo Module Unloaded ===\n");
}

module_init(printk_demo_init);
module_exit(printk_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Kernel printk and Logging Demo");
MODULE_VERSION("1.0");
