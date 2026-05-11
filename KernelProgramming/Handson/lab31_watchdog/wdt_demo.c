/*
 * wdt_demo.c — Software Watchdog Timer Driver
 *
 * Virtual watchdog — logs a warning on expiry instead of rebooting.
 * For safety during development. Real watchdog would trigger hw reset.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/watchdog.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#define DEFAULT_TIMEOUT 10  /* seconds */

struct wdt_demo_priv {
    struct watchdog_device wdd;
    struct timer_list timer;
    bool running;
};

static void wdt_expired(struct timer_list *t)
{
    struct wdt_demo_priv *priv = from_timer(priv, t, timer);
    pr_crit("[wdt_demo] *** WATCHDOG EXPIRED! *** (timeout=%ds)\n",
            priv->wdd.timeout);
    pr_crit("[wdt_demo] In a real system, this would REBOOT!\n");
    /* For safety, we don't actually reboot. A real driver would call:
     * emergency_restart();
     */
}

static int wdt_demo_start(struct watchdog_device *wdd)
{
    struct wdt_demo_priv *priv = watchdog_get_drvdata(wdd);

    mod_timer(&priv->timer, jiffies + wdd->timeout * HZ);
    priv->running = true;
    pr_info("[wdt_demo] Started (timeout=%ds)\n", wdd->timeout);
    return 0;
}

static int wdt_demo_stop(struct watchdog_device *wdd)
{
    struct wdt_demo_priv *priv = watchdog_get_drvdata(wdd);

    del_timer_sync(&priv->timer);
    priv->running = false;
    pr_info("[wdt_demo] Stopped\n");
    return 0;
}

static int wdt_demo_ping(struct watchdog_device *wdd)
{
    struct wdt_demo_priv *priv = watchdog_get_drvdata(wdd);

    mod_timer(&priv->timer, jiffies + wdd->timeout * HZ);
    pr_info("[wdt_demo] Pinged (reset to %ds)\n", wdd->timeout);
    return 0;
}

static int wdt_demo_set_timeout(struct watchdog_device *wdd, unsigned int t)
{
    wdd->timeout = t;
    if (watchdog_active(wdd))
        wdt_demo_ping(wdd);
    pr_info("[wdt_demo] Timeout set to %ds\n", t);
    return 0;
}

static const struct watchdog_info wdt_demo_info = {
    .identity = "Lab Software Watchdog",
    .options  = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
};

static const struct watchdog_ops wdt_demo_ops = {
    .owner       = THIS_MODULE,
    .start       = wdt_demo_start,
    .stop        = wdt_demo_stop,
    .ping        = wdt_demo_ping,
    .set_timeout = wdt_demo_set_timeout,
};

static struct wdt_demo_priv *wdt_priv;
static struct platform_device *wdt_pdev;

static int wdt_demo_probe(struct platform_device *pdev)
{
    struct wdt_demo_priv *priv;
    int ret;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->wdd.info        = &wdt_demo_info;
    priv->wdd.ops         = &wdt_demo_ops;
    priv->wdd.timeout     = DEFAULT_TIMEOUT;
    priv->wdd.min_timeout = 1;
    priv->wdd.max_timeout = 120;
    priv->wdd.parent      = &pdev->dev;

    timer_setup(&priv->timer, wdt_expired, 0);

    watchdog_set_drvdata(&priv->wdd, priv);
    watchdog_init_timeout(&priv->wdd, DEFAULT_TIMEOUT, &pdev->dev);

    ret = devm_watchdog_register_device(&pdev->dev, &priv->wdd);
    if (ret) {
        dev_err(&pdev->dev, "Failed to register watchdog: %d\n", ret);
        return ret;
    }

    wdt_priv = priv;
    dev_info(&pdev->dev, "Watchdog registered (timeout=%ds)\n", DEFAULT_TIMEOUT);
    return 0;
}

static int wdt_demo_remove(struct platform_device *pdev)
{
    struct wdt_demo_priv *priv = platform_get_drvdata(pdev);
    del_timer_sync(&priv->timer);
    dev_info(&pdev->dev, "Watchdog removed\n");
    return 0;
}

static struct platform_driver wdt_demo_driver = {
    .probe  = wdt_demo_probe,
    .remove = wdt_demo_remove,
    .driver = {
        .name = "wdt_demo",
    },
};

static int __init wdt_demo_init(void)
{
    int ret = platform_driver_register(&wdt_demo_driver);
    if (ret) return ret;

    wdt_pdev = platform_device_register_simple("wdt_demo", -1, NULL, 0);
    if (IS_ERR(wdt_pdev)) {
        platform_driver_unregister(&wdt_demo_driver);
        return PTR_ERR(wdt_pdev);
    }
    return 0;
}

static void __exit wdt_demo_exit(void)
{
    platform_device_unregister(wdt_pdev);
    platform_driver_unregister(&wdt_demo_driver);
}

module_init(wdt_demo_init);
module_exit(wdt_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Software Watchdog Timer Demo");
