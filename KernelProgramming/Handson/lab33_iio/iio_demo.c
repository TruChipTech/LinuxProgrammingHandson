/*
 * iio_demo.c — IIO (Industrial I/O) Virtual Sensor Driver
 *
 * Registers a virtual IIO device with temperature and accelerometer channels.
 * No hardware needed — generates synthetic data.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/platform_device.h>
#include <linux/random.h>

struct iio_demo_priv {
    int temp_offset;     /* Calibration offset */
    int sample_count;
};

static const struct iio_chan_spec iio_demo_channels[] = {
    {
        .type = IIO_TEMP,
        .info_mask_separate =
            BIT(IIO_CHAN_INFO_RAW) |
            BIT(IIO_CHAN_INFO_SCALE) |
            BIT(IIO_CHAN_INFO_OFFSET),
    },
    {
        .type = IIO_ACCEL,
        .modified = 1,
        .channel2 = IIO_MOD_X,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
    },
    {
        .type = IIO_ACCEL,
        .modified = 1,
        .channel2 = IIO_MOD_Y,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
    },
    {
        .type = IIO_ACCEL,
        .modified = 1,
        .channel2 = IIO_MOD_Z,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
    },
};

static int iio_demo_read_raw(struct iio_dev *indio_dev,
                              struct iio_chan_spec const *chan,
                              int *val, int *val2, long mask)
{
    struct iio_demo_priv *priv = iio_priv(indio_dev);

    priv->sample_count++;

    switch (mask) {
    case IIO_CHAN_INFO_RAW:
        if (chan->type == IIO_TEMP) {
            /* ~25°C with small random variation */
            *val = 2500 + (get_random_u32() % 100) - 50;
        } else {
            /* Accelerometer: random ±1000 */
            *val = (int)(get_random_u32() % 2000) - 1000;
            /* Z axis has gravity component */
            if (chan->channel2 == IIO_MOD_Z)
                *val += 9800;
        }
        return IIO_VAL_INT;

    case IIO_CHAN_INFO_SCALE:
        if (chan->type == IIO_TEMP) {
            *val = 0;
            *val2 = 10000;   /* 0.01 °C per raw unit */
            return IIO_VAL_INT_PLUS_MICRO;
        } else {
            *val = 0;
            *val2 = 1000;    /* 0.001 g per raw unit */
            return IIO_VAL_INT_PLUS_MICRO;
        }

    case IIO_CHAN_INFO_OFFSET:
        *val = priv->temp_offset;
        return IIO_VAL_INT;
    }

    return -EINVAL;
}

static int iio_demo_write_raw(struct iio_dev *indio_dev,
                               struct iio_chan_spec const *chan,
                               int val, int val2, long mask)
{
    struct iio_demo_priv *priv = iio_priv(indio_dev);

    if (mask == IIO_CHAN_INFO_OFFSET && chan->type == IIO_TEMP) {
        priv->temp_offset = val;
        return 0;
    }

    return -EINVAL;
}

static const struct iio_info iio_demo_info = {
    .read_raw  = iio_demo_read_raw,
    .write_raw = iio_demo_write_raw,
};

static struct platform_device *iio_pdev;

static int iio_demo_probe(struct platform_device *pdev)
{
    struct iio_dev *indio_dev;
    struct iio_demo_priv *priv;

    indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*priv));
    if (!indio_dev)
        return -ENOMEM;

    priv = iio_priv(indio_dev);
    priv->temp_offset = 0;
    priv->sample_count = 0;

    indio_dev->name = "iio_demo_sensor";
    indio_dev->info = &iio_demo_info;
    indio_dev->channels = iio_demo_channels;
    indio_dev->num_channels = ARRAY_SIZE(iio_demo_channels);
    indio_dev->modes = INDIO_DIRECT_MODE;

    platform_set_drvdata(pdev, indio_dev);

    return devm_iio_device_register(&pdev->dev, indio_dev);
}

static struct platform_driver iio_demo_driver = {
    .probe = iio_demo_probe,
    .driver = {
        .name = "iio_demo",
    },
};

static int __init iio_demo_init(void)
{
    int ret;

    ret = platform_driver_register(&iio_demo_driver);
    if (ret) return ret;

    iio_pdev = platform_device_register_simple("iio_demo", -1, NULL, 0);
    if (IS_ERR(iio_pdev)) {
        platform_driver_unregister(&iio_demo_driver);
        return PTR_ERR(iio_pdev);
    }

    pr_info("[iio_demo] Registered — check /sys/bus/iio/devices/\n");
    return 0;
}

static void __exit iio_demo_exit(void)
{
    platform_device_unregister(iio_pdev);
    platform_driver_unregister(&iio_demo_driver);
    pr_info("[iio_demo] Removed\n");
}

module_init(iio_demo_init);
module_exit(iio_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("IIO Virtual Sensor Driver Demo");
