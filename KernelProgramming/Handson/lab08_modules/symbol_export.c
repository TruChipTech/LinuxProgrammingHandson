/*
 * symbol_export.c — Module that exports symbols
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

static int shared_counter = 0;

int my_add(int a, int b)
{
    shared_counter++;
    pr_info("[symbol_export] my_add(%d, %d) = %d (call #%d)\n",
            a, b, a + b, shared_counter);
    return a + b;
}
EXPORT_SYMBOL_GPL(my_add);

int my_multiply(int a, int b)
{
    shared_counter++;
    pr_info("[symbol_export] my_multiply(%d, %d) = %d (call #%d)\n",
            a, b, a * b, shared_counter);
    return a * b;
}
EXPORT_SYMBOL_GPL(my_multiply);

int my_get_call_count(void)
{
    return shared_counter;
}
EXPORT_SYMBOL_GPL(my_get_call_count);

static int __init symbol_export_init(void)
{
    pr_info("[symbol_export] Module loaded — symbols exported\n");
    return 0;
}

static void __exit symbol_export_exit(void)
{
    pr_info("[symbol_export] Module unloaded (total calls: %d)\n", shared_counter);
}

module_init(symbol_export_init);
module_exit(symbol_export_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Symbol Export Demo — Provider");
