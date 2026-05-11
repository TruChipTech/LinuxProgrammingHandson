/*
 * regmap_demo.c — Regmap API Demo with I2C
 *
 * Demonstrates unified register access via regmap:
 * read, write, update_bits, bulk operations, caching.
 *
 * For testing: sudo modprobe i2c-stub chip_addr=0x68
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/of.h>

struct regmap_demo_priv {
    struct i2c_client *client;
    struct regmap *regmap;
};

static bool my_readable(struct device *dev, unsigned int reg)
{
    return reg <= 0x75;  /* All registers up to WHO_AM_I */
}

static bool my_writeable(struct device *dev, unsigned int reg)
{
    /* Read-only registers */
    if (reg == 0x75)  /* WHO_AM_I is read-only */
        return false;
    return reg <= 0x6B;
}

static bool my_volatile(struct device *dev, unsigned int reg)
{
    /* Sensor data registers change frequently — don't cache */
    if (reg >= 0x3B && reg <= 0x48)
        return true;
    return false;
}

static const struct regmap_config my_regmap_config = {
    .reg_bits      = 8,
    .val_bits      = 8,
    .max_register  = 0x75,
    .readable_reg  = my_readable,
    .writeable_reg = my_writeable,
    .volatile_reg  = my_volatile,
    .cache_type    = REGCACHE_RBTREE,
};

static ssize_t regdump_show(struct device *dev, struct device_attribute *attr,
                             char *buf)
{
    struct regmap_demo_priv *priv = dev_get_drvdata(dev);
    unsigned int val;
    int len = 0;
    int reg;

    for (reg = 0; reg <= 0x10 && len < PAGE_SIZE - 32; reg++) {
        if (regmap_read(priv->regmap, reg, &val) == 0)
            len += sysfs_emit_at(buf, len, "0x%02x = 0x%02x\n", reg, val);
    }
    return len;
}
static DEVICE_ATTR_RO(regdump);

static struct attribute *regmap_demo_attrs[] = {
    &dev_attr_regdump.attr,
    NULL,
};
ATTRIBUTE_GROUPS(regmap_demo);

static int regmap_demo_probe(struct i2c_client *client)
{
    struct regmap_demo_priv *priv;
    unsigned int val;
    int ret;

    priv = devm_kzalloc(&client->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->client = client;

    /* Initialize regmap */
    priv->regmap = devm_regmap_init_i2c(client, &my_regmap_config);
    if (IS_ERR(priv->regmap)) {
        dev_err(&client->dev, "regmap init failed: %ld\n",
                PTR_ERR(priv->regmap));
        return PTR_ERR(priv->regmap);
    }

    i2c_set_clientdata(client, priv);

    /* Read WHO_AM_I using regmap */
    ret = regmap_read(priv->regmap, 0x75, &val);
    if (ret)
        dev_warn(&client->dev, "Failed to read WHO_AM_I: %d\n", ret);
    else
        dev_info(&client->dev, "WHO_AM_I = 0x%02x (via regmap)\n", val);

    /* Write using regmap */
    ret = regmap_write(priv->regmap, 0x6B, 0x00);  /* Wake up */
    if (!ret)
        dev_info(&client->dev, "Device woken up (reg 0x6B = 0x00)\n");

    /* Update specific bits without read-modify-write */
    ret = regmap_update_bits(priv->regmap, 0x1C, 0x18, 0x10);
    if (!ret)
        dev_info(&client->dev, "Updated bits [4:3] of reg 0x1C\n");

    /* Bulk read */
    {
        u8 buf[6];
        ret = regmap_bulk_read(priv->regmap, 0x3B, buf, 6);
        if (!ret)
            dev_info(&client->dev, "Bulk read 6 bytes from 0x3B\n");
    }

    /* Create sysfs */
    ret = devm_device_add_groups(&client->dev, regmap_demo_groups);

    dev_info(&client->dev, "Regmap demo driver probed\n");
    return 0;
}

static void regmap_demo_remove(struct i2c_client *client)
{
    dev_info(&client->dev, "Regmap demo removed\n");
}

static const struct i2c_device_id regmap_demo_id[] = {
    { "regmap_demo", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, regmap_demo_id);

static const struct of_device_id regmap_demo_of_match[] = {
    { .compatible = "lab,regmap-demo" },
    { }
};
MODULE_DEVICE_TABLE(of, regmap_demo_of_match);

static struct i2c_driver regmap_demo_driver = {
    .driver = {
        .name           = "regmap_demo",
        .of_match_table = regmap_demo_of_match,
    },
    .probe    = regmap_demo_probe,
    .remove   = regmap_demo_remove,
    .id_table = regmap_demo_id,
};
module_i2c_driver(regmap_demo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Regmap API Demo with I2C");
