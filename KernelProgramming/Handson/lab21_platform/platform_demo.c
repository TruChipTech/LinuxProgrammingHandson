/*
 * platform_demo.c — Platform Driver Demo
 *
 * Matches "lab,mydevice" compatible string from Device Tree.
 * Also supports non-DT matching by driver name.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>

struct my_priv {
    struct platform_device *pdev;
    u32 custom_prop;
    const char *label;
};

static int my_probe(struct platform_device *pdev)
{
    struct my_priv *priv;
    struct device_node *np = pdev->dev.of_node;

    dev_info(&pdev->dev, "Probe called!\n");

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->pdev = pdev;

    /* Read DT properties (if available) */
    if (np) {
        if (of_property_read_u32(np, "my-custom-prop", &priv->custom_prop))
            priv->custom_prop = 0;
        if (of_property_read_string(np, "label", &priv->label))
            priv->label = "unknown";

        dev_info(&pdev->dev, "DT: custom_prop=%u, label=%s\n",
                 priv->custom_prop, priv->label);

        if (of_property_read_bool(np, "my-flag"))
            dev_info(&pdev->dev, "DT: my-flag is SET\n");
    } else {
        dev_info(&pdev->dev, "No DT node — matched by name\n");
    }

    platform_set_drvdata(pdev, priv);

    dev_info(&pdev->dev, "Probe complete: %s\n", dev_name(&pdev->dev));
    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    dev_info(&pdev->dev, "Remove called\n");
    return 0;
}

static const struct of_device_id my_of_match[] = {
    { .compatible = "lab,mydevice" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, my_of_match);

static struct platform_driver my_platform_driver = {
    .probe  = my_probe,
    .remove = my_remove,
    .driver = {
        .name           = "my_platform_driver",
        .of_match_table = my_of_match,
    },
};
module_platform_driver(my_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Platform Driver Demo");
