/*
 * gpio_driver.c — GPIO Platform Driver (LED + Button with IRQ)
 *
 * Uses the descriptor-based GPIO API (gpiod).
 * Requires Device Tree overlay or manual platform device.
 *
 * On RPi: connect LED to GPIO17, button to GPIO27 (with pull-up)
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

struct gpio_demo_priv {
    struct gpio_desc *led_gpio;
    struct gpio_desc *button_gpio;
    int button_irq;
    bool led_state;
    unsigned long last_irq_time;
    unsigned long press_count;
    struct timer_list blink_timer;
};

/* Debounce: ignore IRQs within 200ms */
#define DEBOUNCE_MS 200

static irqreturn_t button_isr(int irq, void *data)
{
    struct gpio_demo_priv *priv = data;
    unsigned long now = jiffies;

    if (time_before(now, priv->last_irq_time + msecs_to_jiffies(DEBOUNCE_MS)))
        return IRQ_HANDLED;

    priv->last_irq_time = now;
    priv->press_count++;

    /* Toggle LED on button press */
    priv->led_state = !priv->led_state;
    gpiod_set_value(priv->led_gpio, priv->led_state);

    pr_info("[gpio] Button pressed! (#%lu) LED=%s\n",
            priv->press_count, priv->led_state ? "ON" : "OFF");

    return IRQ_HANDLED;
}

static void blink_timer_fn(struct timer_list *t)
{
    struct gpio_demo_priv *priv = from_timer(priv, t, blink_timer);

    priv->led_state = !priv->led_state;
    gpiod_set_value(priv->led_gpio, priv->led_state);

    mod_timer(&priv->blink_timer, jiffies + msecs_to_jiffies(500));
}

static int gpio_demo_probe(struct platform_device *pdev)
{
    struct gpio_demo_priv *priv;
    int ret;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    /* Get LED GPIO */
    priv->led_gpio = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
    if (IS_ERR(priv->led_gpio)) {
        dev_err(&pdev->dev, "Failed to get LED GPIO\n");
        return PTR_ERR(priv->led_gpio);
    }

    /* Get Button GPIO (optional) */
    priv->button_gpio = devm_gpiod_get_optional(&pdev->dev, "button",
                                                  GPIOD_IN);
    if (IS_ERR(priv->button_gpio))
        return PTR_ERR(priv->button_gpio);

    if (priv->button_gpio) {
        priv->button_irq = gpiod_to_irq(priv->button_gpio);
        if (priv->button_irq < 0) {
            dev_err(&pdev->dev, "Failed to get button IRQ\n");
            return priv->button_irq;
        }

        ret = devm_request_threaded_irq(&pdev->dev, priv->button_irq,
                                         NULL, button_isr,
                                         IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                                         "gpio_button", priv);
        if (ret) {
            dev_err(&pdev->dev, "Failed to request button IRQ\n");
            return ret;
        }

        dev_info(&pdev->dev, "Button IRQ %d registered\n", priv->button_irq);
    }

    /* Start blink timer (comment out if using button toggle instead) */
    timer_setup(&priv->blink_timer, blink_timer_fn, 0);
    /* mod_timer(&priv->blink_timer, jiffies + msecs_to_jiffies(500)); */

    platform_set_drvdata(pdev, priv);
    dev_info(&pdev->dev, "GPIO demo driver probed\n");
    return 0;
}

static int gpio_demo_remove(struct platform_device *pdev)
{
    struct gpio_demo_priv *priv = platform_get_drvdata(pdev);

    del_timer_sync(&priv->blink_timer);
    gpiod_set_value(priv->led_gpio, 0);
    dev_info(&pdev->dev, "GPIO demo driver removed (presses=%lu)\n",
             priv->press_count);
    return 0;
}

static const struct of_device_id gpio_demo_of_match[] = {
    { .compatible = "lab,gpio-demo" },
    { }
};
MODULE_DEVICE_TABLE(of, gpio_demo_of_match);

static struct platform_driver gpio_demo_driver = {
    .probe  = gpio_demo_probe,
    .remove = gpio_demo_remove,
    .driver = {
        .name           = "gpio_demo_driver",
        .of_match_table = gpio_demo_of_match,
    },
};
module_platform_driver(gpio_demo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("GPIO Platform Driver Demo (LED + Button)");
