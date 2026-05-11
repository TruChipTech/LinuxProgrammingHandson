/*
 * uio_sim.c — Simulated Hardware Device UIO Kernel Module
 *
 * Simulates a device with registers and periodic interrupts.
 * Build: make (in this directory)
 * Load:  sudo insmod uio_sim.ko
 * Unload: sudo rmmod uio_sim
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/uio_driver.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>

#define DEVICE_NAME "uio_sim"
#define MEM_SIZE    4096

/* Register offsets (match userspace definitions) */
#define REG_ID        0x00
#define REG_STATUS    0x04
#define REG_CONTROL   0x08
#define REG_DATA      0x0C
#define REG_IRQ_ACK   0x10

struct uio_sim_dev {
    struct uio_info info;
    u32 *regs;
    struct timer_list timer;
    struct platform_device *pdev;
    unsigned long irq_count;
    bool enabled;
};

static struct uio_sim_dev *sim_dev;

static void uio_sim_timer(struct timer_list *t) {
    struct uio_sim_dev *dev = from_timer(dev, t, timer);

    if (!dev->enabled) return;

    dev->irq_count++;
    dev->regs[REG_STATUS / 4] = 0x01;  /* IRQ pending */

    uio_event_notify(&dev->info);

    /* Re-arm if enabled */
    if (dev->enabled)
        mod_timer(&dev->timer, jiffies + msecs_to_jiffies(1000));
}

static int uio_sim_irqcontrol(struct uio_info *info, s32 irq_on) {
    struct uio_sim_dev *dev = info->priv;
    dev->enabled = !!irq_on;

    if (irq_on)
        mod_timer(&dev->timer, jiffies + msecs_to_jiffies(1000));
    else
        del_timer_sync(&dev->timer);

    return 0;
}

static int __init uio_sim_init(void) {
    int ret;

    /* Create platform device */
    sim_dev = kzalloc(sizeof(*sim_dev), GFP_KERNEL);
    if (!sim_dev) return -ENOMEM;

    sim_dev->pdev = platform_device_register_simple(DEVICE_NAME, -1, NULL, 0);
    if (IS_ERR(sim_dev->pdev)) {
        ret = PTR_ERR(sim_dev->pdev);
        kfree(sim_dev);
        return ret;
    }

    /* Allocate register space */
    sim_dev->regs = kzalloc(MEM_SIZE, GFP_KERNEL);
    if (!sim_dev->regs) {
        platform_device_unregister(sim_dev->pdev);
        kfree(sim_dev);
        return -ENOMEM;
    }

    /* Initialize registers */
    sim_dev->regs[REG_ID / 4]      = 0x53494D44;  /* "SIMD" */
    sim_dev->regs[REG_STATUS / 4]  = 0x00;
    sim_dev->regs[REG_CONTROL / 4] = 0x00;
    sim_dev->regs[REG_DATA / 4]    = 0x00;

    /* Setup UIO */
    sim_dev->info.name = DEVICE_NAME;
    sim_dev->info.version = "1.0";
    sim_dev->info.irq = UIO_IRQ_NONE;
    sim_dev->info.irqcontrol = uio_sim_irqcontrol;
    sim_dev->info.priv = sim_dev;

    sim_dev->info.mem[0].name = "sim_regs";
    sim_dev->info.mem[0].addr = (phys_addr_t)(uintptr_t)sim_dev->regs;
    sim_dev->info.mem[0].size = MEM_SIZE;
    sim_dev->info.mem[0].memtype = UIO_MEM_LOGICAL;
    sim_dev->info.mem[0].internal_addr = sim_dev->regs;

    ret = uio_register_device(&sim_dev->pdev->dev, &sim_dev->info);
    if (ret) {
        kfree(sim_dev->regs);
        platform_device_unregister(sim_dev->pdev);
        kfree(sim_dev);
        return ret;
    }

    /* Start timer */
    timer_setup(&sim_dev->timer, uio_sim_timer, 0);
    sim_dev->enabled = true;
    mod_timer(&sim_dev->timer, jiffies + msecs_to_jiffies(1000));

    pr_info("uio_sim: loaded — simulated device ready\n");
    return 0;
}

static void __exit uio_sim_exit(void) {
    sim_dev->enabled = false;
    del_timer_sync(&sim_dev->timer);
    uio_unregister_device(&sim_dev->info);
    kfree(sim_dev->regs);
    platform_device_unregister(sim_dev->pdev);
    kfree(sim_dev);
    pr_info("uio_sim: unloaded\n");
}

module_init(uio_sim_init);
module_exit(uio_sim_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Simulated Hardware Device — UIO Kernel Module");
