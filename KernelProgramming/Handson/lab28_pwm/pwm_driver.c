/*
 * pwm_driver.c — PWM Consumer Platform Driver
 *
 * Requests a PWM channel and controls duty cycle via sysfs.
 * Creates a "breathing LED" effect using a kernel timer.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/of.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#define PERIOD_NS  1000000   /* 1ms = 1kHz */

struct pwm_demo_priv {
    struct pwm_device *pwm;
    struct timer_list breathe_timer;
    int duty_percent;
    int direction;  /* 1 = brightening, -1 = dimming */
    bool breathing;
};

static ssize_t duty_show(struct device *dev, struct device_attribute *attr,
                          char *buf)
{
    struct pwm_demo_priv *priv = dev_get_drvdata(dev);
    return sysfs_emit(buf, "%d\n", priv->duty_percent);
}

static ssize_t duty_store(struct device *dev, struct device_attribute *attr,
                           const char *buf, size_t count)
{
    struct pwm_demo_priv *priv = dev_get_drvdata(dev);
    int val;

    if (kstrtoint(buf, 10, &val) || val < 0 || val > 100)
        return -EINVAL;

    priv->duty_percent = val;
    pwm_config(priv->pwm, (unsigned long)PERIOD_NS * val / 100, PERIOD_NS);
    return count;
}
static DEVICE_ATTR_RW(duty);

static ssize_t breathe_store(struct device *dev, struct device_attribute *attr,
                              const char *buf, size_t count)
{
    struct pwm_demo_priv *priv = dev_get_drvdata(dev);
    int val;

    if (kstrtoint(buf, 10, &val))
        return -EINVAL;

    if (val && !priv->breathing) {
        priv->breathing = true;
        priv->direction = 1;
        priv->duty_percent = 0;
        mod_timer(&priv->breathe_timer, jiffies + msecs_to_jiffies(20));
        dev_info(dev, "Breathing started\n");
    } else if (!val && priv->breathing) {
        priv->breathing = false;
        del_timer_sync(&priv->breathe_timer);
        dev_info(dev, "Breathing stopped\n");
    }
    return count;
}
static DEVICE_ATTR_WO(breathe);

static struct attribute *pwm_demo_attrs[] = {
    &dev_attr_duty.attr,
    &dev_attr_breathe.attr,
    NULL,
};
ATTRIBUTE_GROUPS(pwm_demo);

static void breathe_timer_fn(struct timer_list *t)
{
    struct pwm_demo_priv *priv = from_timer(priv, t, breathe_timer);

    priv->duty_percent += priv->direction * 2;
    if (priv->duty_percent >= 100)
        priv->direction = -1;
    else if (priv->duty_percent <= 0)
        priv->direction = 1;

    pwm_config(priv->pwm,
               (unsigned long)PERIOD_NS * priv->duty_percent / 100,
               PERIOD_NS);

    if (priv->breathing)
        mod_timer(&priv->breathe_timer, jiffies + msecs_to_jiffies(20));
}

static int pwm_demo_probe(struct platform_device *pdev)
{
    struct pwm_demo_priv *priv;
    int ret;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->pwm = devm_pwm_get(&pdev->dev, NULL);
    if (IS_ERR(priv->pwm)) {
        dev_err(&pdev->dev, "Failed to get PWM: %ld\n", PTR_ERR(priv->pwm));
        return PTR_ERR(priv->pwm);
    }

    /* Configure and enable */
    pwm_config(priv->pwm, 0, PERIOD_NS);
    pwm_enable(priv->pwm);

    timer_setup(&priv->breathe_timer, breathe_timer_fn, 0);

    platform_set_drvdata(pdev, priv);

    ret = devm_device_add_groups(&pdev->dev, pwm_demo_groups);
    if (ret)
        return ret;

    dev_info(&pdev->dev, "PWM demo driver probed\n");
    dev_info(&pdev->dev, "Set duty: echo 50 > /sys/.../duty\n");
    dev_info(&pdev->dev, "Breathe:  echo 1 > /sys/.../breathe\n");
    return 0;
}

static int pwm_demo_remove(struct platform_device *pdev)
{
    struct pwm_demo_priv *priv = platform_get_drvdata(pdev);

    del_timer_sync(&priv->breathe_timer);
    pwm_disable(priv->pwm);
    dev_info(&pdev->dev, "PWM demo driver removed\n");
    return 0;
}

static const struct of_device_id pwm_demo_of_match[] = {
    { .compatible = "lab,pwm-demo" },
    { }
};
MODULE_DEVICE_TABLE(of, pwm_demo_of_match);

static struct platform_driver pwm_demo_driver = {
    .probe  = pwm_demo_probe,
    .remove = pwm_demo_remove,
    .driver = {
        .name           = "pwm_demo_driver",
        .of_match_table = pwm_demo_of_match,
    },
};
module_platform_driver(pwm_demo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("PWM Consumer Platform Driver Demo");
