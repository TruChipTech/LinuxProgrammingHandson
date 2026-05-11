/*
 * rt_demo.c — Real-Time Kernel Programming Demo
 *
 * Demonstrates RT-safe kernel techniques:
 *   - raw_spinlock_t (non-preemptible even on PREEMPT_RT)
 *   - ktime_get() latency measurement
 *   - High-resolution timer with RT considerations
 *
 * Creates /proc/rt_demo for results.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/spinlock.h>
#include <linux/preempt.h>
#include <linux/sched.h>

#define NUM_SAMPLES 100

struct rt_demo_data {
    raw_spinlock_t raw_lock;
    spinlock_t     regular_lock;
    ktime_t raw_lock_times[NUM_SAMPLES];
    ktime_t reg_lock_times[NUM_SAMPLES];
    int sample_count;
    struct hrtimer timer;
    int timer_fires;
    ktime_t timer_latencies[NUM_SAMPLES];
    ktime_t timer_expected;
};

static struct rt_demo_data *demo;

/* Measure raw_spinlock vs regular spinlock acquisition time */
static void measure_lock_latency(struct rt_demo_data *d)
{
    ktime_t start, end;
    int i;

    for (i = 0; i < NUM_SAMPLES; i++) {
        /* raw_spinlock measurement */
        start = ktime_get();
        raw_spin_lock(&d->raw_lock);
        raw_spin_unlock(&d->raw_lock);
        end = ktime_get();
        d->raw_lock_times[i] = ktime_sub(end, start);

        /* regular spinlock measurement */
        start = ktime_get();
        spin_lock(&d->regular_lock);
        spin_unlock(&d->regular_lock);
        end = ktime_get();
        d->reg_lock_times[i] = ktime_sub(end, start);
    }
    d->sample_count = NUM_SAMPLES;
}

/* hrtimer callback — measure scheduling latency */
static enum hrtimer_restart rt_timer_callback(struct hrtimer *timer)
{
    struct rt_demo_data *d = container_of(timer, struct rt_demo_data, timer);
    ktime_t now = ktime_get();

    if (d->timer_fires < NUM_SAMPLES) {
        d->timer_latencies[d->timer_fires] =
            ktime_sub(now, d->timer_expected);
        d->timer_fires++;

        d->timer_expected = ktime_add_us(now, 1000); /* 1ms */
        hrtimer_forward_now(timer, ktime_set(0, 1000000));
        return HRTIMER_RESTART;
    }

    return HRTIMER_NORESTART;
}

static int proc_show(struct seq_file *m, void *v)
{
    int i;
    s64 raw_min = S64_MAX, raw_max = 0, raw_sum = 0;
    s64 reg_min = S64_MAX, reg_max = 0, reg_sum = 0;
    s64 lat_min = S64_MAX, lat_max = 0, lat_sum = 0;
    s64 val;

    seq_puts(m, "=== RT Kernel Programming Demo ===\n\n");

    /* Preemption info */
    seq_printf(m, "Kernel preemption model: %s\n",
#ifdef CONFIG_PREEMPT_RT
               "PREEMPT_RT (Full Real-Time)"
#elif defined(CONFIG_PREEMPT)
               "PREEMPT (Low-Latency Desktop)"
#elif defined(CONFIG_PREEMPT_VOLUNTARY)
               "PREEMPT_VOLUNTARY"
#else
               "PREEMPT_NONE"
#endif
    );
    seq_printf(m, "HZ = %d (tick = %d us)\n\n", HZ, 1000000 / HZ);

    /* Lock latency stats */
    if (demo->sample_count == 0) {
        seq_puts(m, "No samples yet (load module to collect)\n");
        return 0;
    }

    for (i = 0; i < demo->sample_count; i++) {
        val = ktime_to_ns(demo->raw_lock_times[i]);
        if (val < raw_min) raw_min = val;
        if (val > raw_max) raw_max = val;
        raw_sum += val;

        val = ktime_to_ns(demo->reg_lock_times[i]);
        if (val < reg_min) reg_min = val;
        if (val > reg_max) reg_max = val;
        reg_sum += val;
    }

    seq_puts(m, "Lock Acquisition Latency (ns):\n");
    seq_printf(m, "  raw_spinlock: min=%lld  max=%lld  avg=%lld\n",
               raw_min, raw_max, raw_sum / demo->sample_count);
    seq_printf(m, "  spinlock:     min=%lld  max=%lld  avg=%lld\n",
               reg_min, reg_max, reg_sum / demo->sample_count);
    seq_puts(m, "\n");

    /* Timer latency stats */
    if (demo->timer_fires > 0) {
        for (i = 0; i < demo->timer_fires; i++) {
            val = ktime_to_ns(demo->timer_latencies[i]);
            if (val < lat_min) lat_min = val;
            if (val > lat_max) lat_max = val;
            lat_sum += val;
        }
        seq_printf(m, "HRTimer Scheduling Latency (1ms period, %d fires):\n",
                   demo->timer_fires);
        seq_printf(m, "  min=%lld ns  max=%lld ns  avg=%lld ns\n",
                   lat_min, lat_max, lat_sum / demo->timer_fires);
    }

    return 0;
}

static int proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_show, NULL);
}

static const struct proc_ops proc_fops = {
    .proc_open    = proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static struct proc_dir_entry *pentry;

static int __init rt_demo_init(void)
{
    demo = kzalloc(sizeof(*demo), GFP_KERNEL);
    if (!demo)
        return -ENOMEM;

    raw_spin_lock_init(&demo->raw_lock);
    spin_lock_init(&demo->regular_lock);

    /* Measure lock latencies */
    measure_lock_latency(demo);

    /* Start hrtimer for scheduling latency measurement */
    hrtimer_init(&demo->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    demo->timer.function = rt_timer_callback;
    demo->timer_fires = 0;
    demo->timer_expected = ktime_add_us(ktime_get(), 1000);
    hrtimer_start(&demo->timer, ktime_set(0, 1000000), HRTIMER_MODE_REL);

    pentry = proc_create("rt_demo", 0444, NULL, &proc_fops);
    if (!pentry) {
        hrtimer_cancel(&demo->timer);
        kfree(demo);
        return -ENOMEM;
    }

    pr_info("[rt_demo] Loaded — read /proc/rt_demo for results\n");
    return 0;
}

static void __exit rt_demo_exit(void)
{
    hrtimer_cancel(&demo->timer);
    proc_remove(pentry);
    kfree(demo);
    pr_info("[rt_demo] Removed\n");
}

module_init(rt_demo_init);
module_exit(rt_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Real-Time Kernel Programming Demo");
