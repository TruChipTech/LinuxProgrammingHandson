/*
 * irq_demo.c — Interrupt Handling Demo (keyboard IRQ 1)
 *
 * Shares IRQ 1 (keyboard) to demonstrate interrupt handling.
 * On Raspberry Pi, use a GPIO IRQ instead (see threaded_irq_demo.c).
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>

#define SHARED_IRQ 1  /* Keyboard IRQ on x86 */

static int irq_count = 0;
static int dev_id_cookie;  /* Unique dev_id for shared IRQ */

static irqreturn_t my_irq_handler(int irq, void *dev_id)
{
    irq_count++;
    if (irq_count <= 10) {
        pr_info("[irq_demo] IRQ %d handled! (count=%d, CPU=%d)\n",
                irq, irq_count, smp_processor_id());
    }
    return IRQ_NONE;  /* We didn't actually handle it — let real driver proceed */
}

static int __init irq_demo_init(void)
{
    int ret;

    pr_info("[irq_demo] Requesting shared IRQ %d\n", SHARED_IRQ);

    ret = request_irq(SHARED_IRQ, my_irq_handler,
                      IRQF_SHARED, "irq_demo", &dev_id_cookie);
    if (ret) {
        pr_err("[irq_demo] request_irq failed: %d\n", ret);
        pr_info("[irq_demo] Note: On RPi, use GPIO IRQ instead\n");
        return ret;
    }

    pr_info("[irq_demo] Registered on IRQ %d — press keys to trigger\n",
            SHARED_IRQ);
    return 0;
}

static void __exit irq_demo_exit(void)
{
    free_irq(SHARED_IRQ, &dev_id_cookie);
    pr_info("[irq_demo] Unregistered (total IRQs: %d)\n", irq_count);
}

module_init(irq_demo_init);
module_exit(irq_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Interrupt Handling Demo");
