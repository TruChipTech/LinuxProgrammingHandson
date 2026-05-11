/*
 * rtc_demo.c — Virtual RTC Driver (Software-Only)
 *
 * Implements a software RTC that stores time as an offset from
 * the system clock. No hardware needed.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/of.h>

struct rtc_demo_priv {
    struct rtc_device *rtc;
    s64 offset_secs;  /* offset from system time */
};

static int rtc_demo_read_time(struct device *dev, struct rtc_time *tm)
{
    struct rtc_demo_priv *priv = dev_get_drvdata(dev);
    struct timespec64 ts;

    ktime_get_real_ts64(&ts);
    rtc_time64_to_tm(ts.tv_sec + priv->offset_secs, tm);

    dev_dbg(dev, "Read time: %ptR\n", tm);
    return 0;
}

static int rtc_demo_set_time(struct device *dev, struct rtc_time *tm)
{
    struct rtc_demo_priv *priv = dev_get_drvdata(dev);
    struct timespec64 ts;
    time64_t new_time;

    new_time = rtc_tm_to_time64(tm);
    ktime_get_real_ts64(&ts);
    priv->offset_secs = new_time - ts.tv_sec;

    dev_info(dev, "Set time: %ptR (offset=%lld)\n", tm, priv->offset_secs);
    return 0;
}

static const struct rtc_class_ops rtc_demo_ops = {
    .read_time = rtc_demo_read_time,
    .set_time  = rtc_demo_set_time,
};

static int rtc_demo_probe(struct platform_device *pdev)
{
    struct rtc_demo_priv *priv;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->offset_secs = 0;
    platform_set_drvdata(pdev, priv);

    priv->rtc = devm_rtc_device_register(&pdev->dev, "rtc_demo",
                                          &rtc_demo_ops, THIS_MODULE);
    if (IS_ERR(priv->rtc)) {
        dev_err(&pdev->dev, "Failed to register RTC\n");
        return PTR_ERR(priv->rtc);
    }

    dev_info(&pdev->dev, "Virtual RTC registered\n");
    dev_info(&pdev->dev, "Test: hwclock --rtc /dev/rtc1 --show\n");
    return 0;
}

static int rtc_demo_remove(struct platform_device *pdev)
{
    dev_info(&pdev->dev, "Virtual RTC removed\n");
    return 0;
}

static const struct of_device_id rtc_demo_of_match[] = {
    { .compatible = "lab,rtc-demo" },
    { }
};
MODULE_DEVICE_TABLE(of, rtc_demo_of_match);

static struct platform_driver rtc_demo_driver = {
    .probe  = rtc_demo_probe,
    .remove = rtc_demo_remove,
    .driver = {
        .name           = "rtc_demo",
        .of_match_table = rtc_demo_of_match,
    },
};

/* Also create a platform device for easy testing without DT */
static struct platform_device *rtc_pdev;

static int __init rtc_demo_init(void)
{
    int ret;

    ret = platform_driver_register(&rtc_demo_driver);
    if (ret)
        return ret;

    /* Auto-create device for testing */
    rtc_pdev = platform_device_register_simple("rtc_demo", -1, NULL, 0);
    if (IS_ERR(rtc_pdev)) {
        platform_driver_unregister(&rtc_demo_driver);
        return PTR_ERR(rtc_pdev);
    }

    return 0;
}

static void __exit rtc_demo_exit(void)
{
    platform_device_unregister(rtc_pdev);
    platform_driver_unregister(&rtc_demo_driver);
}

module_init(rtc_demo_init);
module_exit(rtc_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Virtual RTC Driver Demo");
