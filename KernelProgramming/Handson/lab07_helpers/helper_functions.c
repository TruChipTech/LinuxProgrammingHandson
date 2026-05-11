/*
 * helper_functions.c — Kernel Helper Functions Demo Module
 *
 * Demonstrates kernel-specific helper functions.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/math.h>

static int __init helpers_init(void)
{
    char buf[128];
    int val;
    unsigned long ulval;

    pr_info("=== Kernel Helper Functions Demo ===\n");

    /* --- String-to-integer conversions --- */
    pr_info("--- kstrtoint / kstrtoul ---\n");
    if (kstrtoint("12345", 10, &val) == 0)
        pr_info("  kstrtoint(\"12345\") = %d\n", val);
    if (kstrtoint("-99", 10, &val) == 0)
        pr_info("  kstrtoint(\"-99\") = %d\n", val);
    if (kstrtoul("0xFF", 16, &ulval) == 0)
        pr_info("  kstrtoul(\"0xFF\", 16) = %lu\n", ulval);
    if (kstrtoint("not_a_number", 10, &val) != 0)
        pr_info("  kstrtoint(\"not_a_number\") = FAIL (expected)\n");

    /* --- snprintf --- */
    pr_info("--- snprintf ---\n");
    snprintf(buf, sizeof(buf), "Kernel %s on %s",
             utsname()->release, utsname()->machine);
    pr_info("  Formatted: %s\n", buf);

    /* --- Memory operations --- */
    pr_info("--- Memory operations ---\n");
    {
        char a[16], b[16];
        memset(a, 'A', sizeof(a));
        memcpy(b, a, sizeof(b));
        pr_info("  memcmp(a, b) = %d (0 = equal)\n", memcmp(a, b, sizeof(a)));
        b[0] = 'B';
        pr_info("  memcmp(a, b) after change = %d (non-zero)\n",
                memcmp(a, b, sizeof(a)));
    }

    /* --- String operations --- */
    pr_info("--- String operations ---\n");
    {
        char s1[] = "Hello Kernel";
        pr_info("  strlen(\"%s\") = %zu\n", s1, strlen(s1));
        pr_info("  strcmp(\"abc\", \"abc\") = %d\n", strcmp("abc", "abc"));
        pr_info("  strcmp(\"abc\", \"abd\") = %d\n", strcmp("abc", "abd"));

        char *found = strstr(s1, "Kernel");
        if (found)
            pr_info("  strstr found: %s\n", found);
    }

    /* --- Math helpers --- */
    pr_info("--- Math helpers ---\n");
    pr_info("  min(5, 10) = %d\n", min(5, 10));
    pr_info("  max(5, 10) = %d\n", max(5, 10));
    pr_info("  clamp(15, 0, 10) = %d\n", clamp_val(15, 0, 10));
    pr_info("  clamp(-5, 0, 10) = %d\n", clamp_val(-5, 0, 10));
    pr_info("  DIV_ROUND_UP(10, 3) = %lu\n", DIV_ROUND_UP(10, 3));
    pr_info("  abs(-42) = %d\n", abs(-42));

    /* --- Error pointer idiom --- */
    pr_info("--- ERR_PTR / IS_ERR ---\n");
    {
        void *ptr = ERR_PTR(-ENOMEM);
        pr_info("  IS_ERR(ERR_PTR(-ENOMEM)) = %d\n", IS_ERR(ptr) ? 1 : 0);
        pr_info("  PTR_ERR() = %ld (= -ENOMEM = %d)\n", PTR_ERR(ptr), -ENOMEM);

        ptr = kmalloc(64, GFP_KERNEL);
        pr_info("  IS_ERR(kmalloc result) = %d\n", IS_ERR(ptr) ? 1 : 0);
        kfree(ptr);
    }

    /* --- Time helpers --- */
    pr_info("--- Time helpers ---\n");
    pr_info("  HZ = %d\n", HZ);
    pr_info("  jiffies = %lu\n", jiffies);
    pr_info("  msecs_to_jiffies(1000) = %lu\n", msecs_to_jiffies(1000));
    pr_info("  jiffies_to_msecs(HZ) = %u\n", jiffies_to_msecs(HZ));
    {
        unsigned long start = jiffies;
        mdelay(5);  /* Busy-wait 5ms */
        unsigned long elapsed = jiffies - start;
        pr_info("  mdelay(5): elapsed %lu jiffies (%u ms)\n",
                elapsed, jiffies_to_msecs(elapsed));
    }

    /* --- BUG_ON / WARN_ON --- */
    pr_info("--- Assertions ---\n");
    WARN_ON(1 == 2);  /* Should NOT trigger */
    pr_info("  WARN_ON(1 == 2) — no warning (correct)\n");
    WARN_ON(1 == 1);  /* WILL trigger a warning in dmesg */
    pr_info("  WARN_ON(1 == 1) — warning triggered (expected)\n");
    /* BUG_ON() would crash the kernel — don't use in demos! */

    pr_info("=== Helper Functions Demo Complete ===\n");
    return 0;
}

static void __exit helpers_exit(void)
{
    pr_info("=== Helper Functions Demo Unloaded ===\n");
}

module_init(helpers_init);
module_exit(helpers_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Kernel Helper Functions Demo");
