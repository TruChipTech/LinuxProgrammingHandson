/*
 * ktimer_demo.c — Kernel Timer Demo Module
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/moduleparam.h>

static int interval_ms = 1000;
module_param(interval_ms, int, 0644);
MODULE_PARM_DESC(interval_ms, "Timer interval in milliseconds (default 1000)");

static struct timer_list my_timer;
static unsigned long tick_count = 0;

static void timer_callback(struct timer_list *t)
{
    tick_count++;
    pr_info("[ktimer] Tick #%lu (jiffies=%lu, interval=%dms)\n",
            tick_count, jiffies, interval_ms);

    /* Re-arm the timer */
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(interval_ms));
}

static int __init ktimer_demo_init(void)
{
    pr_info("[ktimer] Module loaded (interval=%dms, HZ=%d)\n", interval_ms, HZ);

    timer_setup(&my_timer, timer_callback, 0);
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(interval_ms));

    return 0;
}

static void __exit ktimer_demo_exit(void)
{
    del_timer_sync(&my_timer);
    pr_info("[ktimer] Module unloaded after %lu ticks\n", tick_count);
}

module_init(ktimer_demo_init);
module_exit(ktimer_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Kernel Timer Demo");
