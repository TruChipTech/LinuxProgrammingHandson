/*
 * input_demo.c — Input Subsystem Demo (Virtual Keyboard)
 *
 * Generates periodic key events visible via evtest.
 * Also demonstrates GPIO button → input event integration.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

static struct input_dev *virt_kbd;
static struct timer_list key_timer;
static int key_idx;

static const unsigned int demo_keys[] = {
    KEY_H, KEY_E, KEY_L, KEY_L, KEY_O, KEY_SPACE
};
#define NUM_KEYS ARRAY_SIZE(demo_keys)

static void key_timer_fn(struct timer_list *t)
{
    unsigned int key = demo_keys[key_idx % NUM_KEYS];

    /* Press */
    input_report_key(virt_kbd, key, 1);
    input_sync(virt_kbd);

    /* Release (after tiny delay in same callback for simplicity) */
    input_report_key(virt_kbd, key, 0);
    input_sync(virt_kbd);

    pr_info("[input] Sent key %d (#%d)\n", key, key_idx);
    key_idx++;

    if (key_idx < 30)  /* Stop after 30 keys */
        mod_timer(&key_timer, jiffies + msecs_to_jiffies(500));
}

static int __init input_demo_init(void)
{
    int i, ret;

    virt_kbd = input_allocate_device();
    if (!virt_kbd)
        return -ENOMEM;

    virt_kbd->name = "Lab Virtual Keyboard";
    virt_kbd->phys = "lab/input0";
    virt_kbd->id.bustype = BUS_VIRTUAL;
    virt_kbd->id.vendor  = 0x0001;
    virt_kbd->id.product = 0x0001;
    virt_kbd->id.version = 0x0001;

    /* Set capabilities */
    set_bit(EV_KEY, virt_kbd->evbit);
    for (i = 0; i < NUM_KEYS; i++)
        set_bit(demo_keys[i], virt_kbd->keybit);
    /* Also allow KEY_ENTER for button demo */
    set_bit(KEY_ENTER, virt_kbd->keybit);

    ret = input_register_device(virt_kbd);
    if (ret) {
        input_free_device(virt_kbd);
        return ret;
    }

    pr_info("[input] Virtual keyboard registered\n");
    pr_info("[input] Test: evtest /dev/input/eventN\n");

    /* Start sending keys */
    timer_setup(&key_timer, key_timer_fn, 0);
    mod_timer(&key_timer, jiffies + msecs_to_jiffies(1000));

    return 0;
}

static void __exit input_demo_exit(void)
{
    del_timer_sync(&key_timer);
    input_unregister_device(virt_kbd);
    pr_info("[input] Virtual keyboard removed\n");
}

module_init(input_demo_init);
module_exit(input_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Input Subsystem Demo (Virtual Keyboard)");
