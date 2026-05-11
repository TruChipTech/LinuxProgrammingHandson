/*
 * debugfs_demo.c — Debugfs Interface Demo Module
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/jiffies.h>

static struct dentry *debug_dir;
static u32 counter = 0;
static bool enable = true;
static char blob_data[128] = "Hello from debugfs blob!";

static int __init debugfs_demo_init(void)
{
    debug_dir = debugfs_create_dir("mydriver_debug", NULL);
    if (IS_ERR(debug_dir))
        return PTR_ERR(debug_dir);

    /* Simple types — debugfs handles read/write automatically */
    debugfs_create_u32("counter", 0666, debug_dir, &counter);
    debugfs_create_bool("enable", 0666, debug_dir, &enable);

    /* Blob — read-only binary data */
    {
        static struct debugfs_blob_wrapper blob;
        blob.data = blob_data;
        blob.size = strlen(blob_data);
        debugfs_create_blob("blob_data", 0444, debug_dir, &blob);
    }

    /* Read-only types */
    debugfs_create_x32("jiffies_hex", 0444, debug_dir, (u32 *)&jiffies);
    debugfs_create_size_t("page_size", 0444, debug_dir,
                          (size_t[]){PAGE_SIZE});

    pr_info("[debugfs_demo] Module loaded — /sys/kernel/debug/mydriver_debug/\n");
    return 0;
}

static void __exit debugfs_demo_exit(void)
{
    debugfs_remove_recursive(debug_dir);
    pr_info("[debugfs_demo] Module unloaded\n");
}

module_init(debugfs_demo_init);
module_exit(debugfs_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Debugfs Interface Demo");
