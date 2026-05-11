/*
 * symbol_import.c — Module that uses exported symbols
 *
 * Must load symbol_export.ko first!
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

/* Declare external symbols (exported by symbol_export module) */
extern int my_add(int a, int b);
extern int my_multiply(int a, int b);
extern int my_get_call_count(void);

static int __init symbol_import_init(void)
{
    int result;

    pr_info("[symbol_import] Module loaded — testing imported symbols\n");

    result = my_add(10, 20);
    pr_info("[symbol_import] my_add(10, 20) = %d\n", result);

    result = my_multiply(5, 7);
    pr_info("[symbol_import] my_multiply(5, 7) = %d\n", result);

    pr_info("[symbol_import] Total calls so far: %d\n", my_get_call_count());

    return 0;
}

static void __exit symbol_import_exit(void)
{
    pr_info("[symbol_import] Module unloaded\n");
}

module_init(symbol_import_init);
module_exit(symbol_import_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Symbol Import Demo — Consumer");
