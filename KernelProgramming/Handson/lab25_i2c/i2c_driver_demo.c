/*
 * i2c_driver_demo.c — I2C Client Driver Demo
 *
 * Generic I2C device driver that reads a device ID register.
 * Works with MPU6050 (addr 0x68, WHO_AM_I at reg 0x75)
 * or any I2C device with an ID register.
 *
 * For testing without hardware: sudo modprobe i2c-stub chip_addr=0x68
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/sysfs.h>

#define WHO_AM_I_REG 0x75

struct my_i2c_priv {
    struct i2c_client *client;
    u8 device_id;
};

/* Sysfs: read a register */
static ssize_t device_id_show(struct device *dev, struct device_attribute *attr,
                               char *buf)
{
    struct my_i2c_priv *priv = dev_get_drvdata(dev);
    int val;

    val = i2c_smbus_read_byte_data(priv->client, WHO_AM_I_REG);
    if (val < 0)
        return val;

    return sysfs_emit(buf, "0x%02x\n", val);
}
static DEVICE_ATTR_RO(device_id);

/* Sysfs: read arbitrary register */
static ssize_t reg_read_store(struct device *dev, struct device_attribute *attr,
                               const char *buf, size_t count)
{
    struct my_i2c_priv *priv = dev_get_drvdata(dev);
    unsigned int reg;
    int val;

    if (kstrtouint(buf, 0, &reg) || reg > 0xFF)
        return -EINVAL;

    val = i2c_smbus_read_byte_data(priv->client, reg);
    if (val < 0)
        return val;

    dev_info(dev, "Register 0x%02x = 0x%02x\n", reg, val);
    return count;
}
static DEVICE_ATTR_WO(reg_read);

static struct attribute *my_i2c_attrs[] = {
    &dev_attr_device_id.attr,
    &dev_attr_reg_read.attr,
    NULL,
};
ATTRIBUTE_GROUPS(my_i2c);

static int my_i2c_probe(struct i2c_client *client)
{
    struct my_i2c_priv *priv;
    int val;

    dev_info(&client->dev, "Probe: addr=0x%02x\n", client->addr);

    priv = devm_kzalloc(&client->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->client = client;

    /* Read device ID */
    val = i2c_smbus_read_byte_data(client, WHO_AM_I_REG);
    if (val < 0) {
        dev_warn(&client->dev, "Failed to read WHO_AM_I: %d\n", val);
        priv->device_id = 0;
    } else {
        priv->device_id = val;
        dev_info(&client->dev, "WHO_AM_I = 0x%02x\n", val);
    }

    i2c_set_clientdata(client, priv);

    /* Create sysfs attributes */
    return devm_device_add_groups(&client->dev, my_i2c_groups);
}

static void my_i2c_remove(struct i2c_client *client)
{
    dev_info(&client->dev, "Remove\n");
}

static const struct i2c_device_id my_i2c_id[] = {
    { "my_i2c_sensor", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, my_i2c_id);

static const struct of_device_id my_i2c_of_match[] = {
    { .compatible = "lab,my-i2c-sensor" },
    { }
};
MODULE_DEVICE_TABLE(of, my_i2c_of_match);

static struct i2c_driver my_i2c_driver = {
    .driver = {
        .name           = "my_i2c_sensor",
        .of_match_table = my_i2c_of_match,
    },
    .probe    = my_i2c_probe,
    .remove   = my_i2c_remove,
    .id_table = my_i2c_id,
};
module_i2c_driver(my_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("I2C Client Driver Demo");
