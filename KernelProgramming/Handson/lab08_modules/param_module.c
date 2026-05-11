/*
 * param_module.c — Module Parameters Demo
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>

static int myint = 0;
static char *mystr = "default";
static int myarr[4] = {0};
static int arr_count = 0;

module_param(myint, int, 0644);    /* rw: owner+group read, others read */
MODULE_PARM_DESC(myint, "An integer parameter");

module_param(mystr, charp, 0444);  /* read-only */
MODULE_PARM_DESC(mystr, "A string parameter");

module_param_array(myarr, int, &arr_count, 0444);
MODULE_PARM_DESC(myarr, "An array of integers (max 4)");

static int __init param_init(void)
{
    int i;

    pr_info("=== Module Parameters Demo ===\n");
    pr_info("  myint = %d\n", myint);
    pr_info("  mystr = %s\n", mystr);
    pr_info("  myarr[%d] = ", arr_count);
    for (i = 0; i < arr_count; i++)
        pr_cont("%d ", myarr[i]);
    pr_cont("\n");

    pr_info("Try: cat /sys/module/param_module/parameters/myint\n");
    pr_info("Try: echo 99 > /sys/module/param_module/parameters/myint\n");

    return 0;
}

static void __exit param_exit(void)
{
    pr_info("param_module unloaded (myint was %d)\n", myint);
}

module_init(param_init);
module_exit(param_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Module Parameters Demo");
