/*
 * serdev_demo.c — Serial Device (serdev) Driver Demo
 *
 * Modern kernel approach for serial-attached devices.
 * Replaces TTY line disciplines for structured serial communication.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/serdev.h>
#include <linux/of.h>

struct serdev_demo_priv {
    struct serdev_device *serdev;
    char rx_buf[256];
    int rx_len;
};

static ssize_t serdev_demo_receive(struct serdev_device *serdev,
                                    const u8 *buf, size_t count)
{
    struct serdev_demo_priv *priv = serdev_device_get_drvdata(serdev);
    int to_copy = min_t(int, count, sizeof(priv->rx_buf) - priv->rx_len - 1);

    if (to_copy > 0) {
        memcpy(priv->rx_buf + priv->rx_len, buf, to_copy);
        priv->rx_len += to_copy;
        priv->rx_buf[priv->rx_len] = '\0';
    }

    dev_info(&serdev->dev, "RX %zu bytes: %.*s\n", count, (int)count, buf);
    return count;
}

static void serdev_demo_write_wakeup(struct serdev_device *serdev)
{
    dev_dbg(&serdev->dev, "Write wakeup\n");
}

static const struct serdev_device_ops serdev_demo_ops = {
    .receive_buf   = serdev_demo_receive,
    .write_wakeup  = serdev_demo_write_wakeup,
};

static int serdev_demo_probe(struct serdev_device *serdev)
{
    struct serdev_demo_priv *priv;
    int ret;

    dev_info(&serdev->dev, "Probe\n");

    priv = devm_kzalloc(&serdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->serdev = serdev;
    serdev_device_set_drvdata(serdev, priv);

    ret = serdev_device_open(serdev);
    if (ret) {
        dev_err(&serdev->dev, "Failed to open serdev: %d\n", ret);
        return ret;
    }

    serdev_device_set_baudrate(serdev, 9600);
    serdev_device_set_flow_control(serdev, false);
    serdev_device_set_client_ops(serdev, &serdev_demo_ops);

    /* Send a test string */
    serdev_device_write_buf(serdev, "AT\r\n", 4);
    dev_info(&serdev->dev, "Sent 'AT' at 9600 baud\n");

    return 0;
}

static void serdev_demo_remove(struct serdev_device *serdev)
{
    serdev_device_close(serdev);
    dev_info(&serdev->dev, "Remove\n");
}

static const struct of_device_id serdev_demo_of_match[] = {
    { .compatible = "lab,my-serial-device" },
    { }
};
MODULE_DEVICE_TABLE(of, serdev_demo_of_match);

static struct serdev_device_driver serdev_demo_driver = {
    .driver = {
        .name           = "serdev_demo",
        .of_match_table = serdev_demo_of_match,
    },
    .probe  = serdev_demo_probe,
    .remove = serdev_demo_remove,
};
module_serdev_device_driver(serdev_demo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Serial Device (serdev) Driver Demo");
