/*
 * threaded_irq_demo.c — Threaded IRQ Demo
 *
 * Uses a simulated IRQ via a timer + softirq model.
 * On Raspberry Pi: modify to use gpio_to_irq() for a real GPIO interrupt.
 *
 * RPi GPIO example (commented out):
 *   #include <linux/gpio.h>
 *   #define GPIO_PIN 17
 *   irq_num = gpio_to_irq(GPIO_PIN);
 *   request_threaded_irq(irq_num, hardirq_fn, thread_fn,
 *                        IRQF_TRIGGER_RISING | IRQF_ONESHOT,
 *                        "gpio_threaded_irq", &dev_id);
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/workqueue.h>

static int irq_line = 1;  /* Default: keyboard IRQ */
module_param(irq_line, int, 0444);
MODULE_PARM_DESC(irq_line, "IRQ line to attach (default 1 = keyboard)");

static int hard_count = 0;
static int thread_count = 0;
static int dev_id_cookie;

/*
 * Hard IRQ handler — runs in interrupt context (fast, no sleeping).
 * Return IRQ_WAKE_THREAD to schedule the threaded handler.
 */
static irqreturn_t hardirq_handler(int irq, void *dev_id)
{
    hard_count++;
    /* Minimal work in hardirq — just acknowledge and defer */
    return IRQ_WAKE_THREAD;
}

/*
 * Threaded handler — runs in process context (can sleep!).
 * This is the "bottom half" managed by the kernel's IRQ thread.
 */
static irqreturn_t thread_handler(int irq, void *dev_id)
{
    thread_count++;
    if (thread_count <= 10) {
        pr_info("[threaded_irq] Thread handler (PID=%d, CPU=%d): "
                "hard=%d, thread=%d\n",
                current->pid, smp_processor_id(),
                hard_count, thread_count);
    }
    return IRQ_HANDLED;
}

static int __init threaded_irq_demo_init(void)
{
    int ret;

    pr_info("[threaded_irq] Requesting threaded IRQ %d\n", irq_line);

    ret = request_threaded_irq(irq_line,
                               hardirq_handler,
                               thread_handler,
                               IRQF_SHARED | IRQF_ONESHOT,
                               "threaded_irq_demo",
                               &dev_id_cookie);
    if (ret) {
        pr_err("[threaded_irq] Failed: %d\n", ret);
        return ret;
    }

    pr_info("[threaded_irq] Registered — hard IRQ -> thread handler\n");
    pr_info("[threaded_irq] Press keys to trigger (x86) or toggle GPIO (RPi)\n");
    return 0;
}

static void __exit threaded_irq_demo_exit(void)
{
    free_irq(irq_line, &dev_id_cookie);
    pr_info("[threaded_irq] Unregistered (hard=%d, thread=%d)\n",
            hard_count, thread_count);
}

module_init(threaded_irq_demo_init);
module_exit(threaded_irq_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Threaded IRQ Demo");
