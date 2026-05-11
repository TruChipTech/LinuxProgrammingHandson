/*
 * uio_demo.c — Minimal UIO Kernel Module
 *
 * Registers a basic UIO device with a timer-based interrupt.
 * Build: make (in this directory)
 * Load:  sudo insmod uio_demo.ko
 * Check: ls /dev/uio*
 * Unload: sudo rmmod uio_demo
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/uio_driver.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#define DEVICE_NAME "uio_demo"
#define MEM_SIZE    4096
#define IRQ_INTERVAL_MS 2000  /* Generate IRQ every 2 seconds */

struct uio_demo_dev {
    struct uio_info info;
    void *mem;
    struct timer_list timer;
    unsigned long irq_count;
};

static struct uio_demo_dev *demo_dev;

/* Timer callback — simulates a hardware interrupt */
static void uio_demo_timer(struct timer_list *t) {
    struct uio_demo_dev *dev = from_timer(dev, t, timer);

    dev->irq_count++;

    /* Write IRQ count to shared memory */
    if (dev->mem) {
        ((u32 *)dev->mem)[0] = 0x44454D4F;      /* "DEMO" magic */
        ((u32 *)dev->mem)[1] = dev->irq_count;   /* IRQ count */
    }

    /* Notify userspace */
    uio_event_notify(&dev->info);

    pr_info("uio_demo: timer IRQ #%lu\n", dev->irq_count);

    /* Re-arm timer */
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(IRQ_INTERVAL_MS));
}

static int uio_demo_irqcontrol(struct uio_info *info, s32 irq_on) {
    struct uio_demo_dev *dev = info->priv;

    if (irq_on) {
        mod_timer(&dev->timer, jiffies + msecs_to_jiffies(IRQ_INTERVAL_MS));
        pr_info("uio_demo: IRQ enabled\n");
    } else {
        del_timer_sync(&dev->timer);
        pr_info("uio_demo: IRQ disabled\n");
    }
    return 0;
}

static int __init uio_demo_init(void) {
    int ret;

    demo_dev = kzalloc(sizeof(*demo_dev), GFP_KERNEL);
    if (!demo_dev)
        return -ENOMEM;

    /* Allocate shared memory region */
    demo_dev->mem = kzalloc(MEM_SIZE, GFP_KERNEL);
    if (!demo_dev->mem) {
        kfree(demo_dev);
        return -ENOMEM;
    }

    /* Initialize magic value */
    ((u32 *)demo_dev->mem)[0] = 0x44454D4F; /* "DEMO" */

    /* Setup UIO info */
    demo_dev->info.name = DEVICE_NAME;
    demo_dev->info.version = "1.0";
    demo_dev->info.irq = UIO_IRQ_NONE;
    demo_dev->info.irqcontrol = uio_demo_irqcontrol;
    demo_dev->info.priv = demo_dev;

    /* Memory mapping */
    demo_dev->info.mem[0].name = "registers";
    demo_dev->info.mem[0].addr = (phys_addr_t)(uintptr_t)demo_dev->mem;
    demo_dev->info.mem[0].size = MEM_SIZE;
    demo_dev->info.mem[0].memtype = UIO_MEM_LOGICAL;
    demo_dev->info.mem[0].internal_addr = demo_dev->mem;

    /* Register UIO device */
    ret = uio_register_device(NULL, &demo_dev->info);
    if (ret) {
        pr_err("uio_demo: registration failed (%d)\n", ret);
        kfree(demo_dev->mem);
        kfree(demo_dev);
        return ret;
    }

    /* Setup timer */
    timer_setup(&demo_dev->timer, uio_demo_timer, 0);
    mod_timer(&demo_dev->timer, jiffies + msecs_to_jiffies(IRQ_INTERVAL_MS));

    pr_info("uio_demo: loaded (IRQ every %dms)\n", IRQ_INTERVAL_MS);
    return 0;
}

static void __exit uio_demo_exit(void) {
    del_timer_sync(&demo_dev->timer);
    uio_unregister_device(&demo_dev->info);
    kfree(demo_dev->mem);
    kfree(demo_dev);
    pr_info("uio_demo: unloaded\n");
}

module_init(uio_demo_init);
module_exit(uio_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Minimal UIO Demo Kernel Module");
