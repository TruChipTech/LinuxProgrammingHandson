/*
 * hrtimer_demo.c — High-Resolution Timer Demo Module
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

static struct hrtimer my_hrtimer;
static ktime_t interval;
static unsigned long tick_count = 0;
static ktime_t last_time;

static enum hrtimer_restart hrtimer_callback(struct hrtimer *timer)
{
    ktime_t now = ktime_get();
    s64 elapsed_ns = ktime_to_ns(ktime_sub(now, last_time));

    tick_count++;
    if (tick_count <= 10) {
        pr_info("[hrtimer] Tick #%lu — elapsed %lld ns (expected 100000000)\n",
                tick_count, elapsed_ns);
    }

    last_time = now;

    if (tick_count >= 20) {
        pr_info("[hrtimer] Stopping after %lu ticks\n", tick_count);
        return HRTIMER_NORESTART;
    }

    hrtimer_forward_now(timer, interval);
    return HRTIMER_RESTART;
}

static int __init hrtimer_demo_init(void)
{
    pr_info("[hrtimer] Module loaded — 100ms high-resolution timer\n");

    interval = ktime_set(0, 100000000);  /* 100 ms in nanoseconds */
    last_time = ktime_get();

    hrtimer_init(&my_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    my_hrtimer.function = hrtimer_callback;
    hrtimer_start(&my_hrtimer, interval, HRTIMER_MODE_REL);

    return 0;
}

static void __exit hrtimer_demo_exit(void)
{
    hrtimer_cancel(&my_hrtimer);
    pr_info("[hrtimer] Module unloaded after %lu ticks\n", tick_count);
}

module_init(hrtimer_demo_init);
module_exit(hrtimer_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("High-Resolution Timer Demo");
