/*
 * pci_demo.c — PCI Device Driver Skeleton
 *
 * Binds to Intel e1000 NIC (default in QEMU).
 * Maps BAR0 and reads status registers.
 *
 * Test in QEMU: unbind the real driver first
 *   echo "0000:00:03.0" > /sys/bus/pci/drivers/e1000/unbind
 *   insmod pci_demo.ko
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>

struct pci_demo_priv {
    void __iomem *bar0;
    struct pci_dev *pdev;
};

static int my_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    struct pci_demo_priv *priv;
    int ret;
    u32 status;

    dev_info(&pdev->dev, "=== PCI Device Probed ===\n");
    dev_info(&pdev->dev, "VID:DID = %04x:%04x\n",
             pdev->vendor, pdev->device);
    dev_info(&pdev->dev, "Class: %06x, IRQ: %d\n",
             pdev->class, pdev->irq);

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->pdev = pdev;

    /* Enable the PCI device */
    ret = pcim_enable_device(pdev);
    if (ret) {
        dev_err(&pdev->dev, "pcim_enable_device failed: %d\n", ret);
        return ret;
    }

    /* Request BAR0 region */
    ret = pcim_iomap_regions(pdev, BIT(0), "pci_demo");
    if (ret) {
        dev_err(&pdev->dev, "pcim_iomap_regions failed: %d\n", ret);
        return ret;
    }

    priv->bar0 = pcim_iomap_table(pdev)[0];
    dev_info(&pdev->dev, "BAR0: phys=%pa, size=%llu, mapped=%p\n",
             &pci_resource_start(pdev, 0),
             (unsigned long long)pci_resource_len(pdev, 0),
             priv->bar0);

    /* List all BARs */
    for (int i = 0; i < 6; i++) {
        if (pci_resource_len(pdev, i))
            dev_info(&pdev->dev, "  BAR%d: start=%pa len=%llu flags=0x%lx\n",
                     i, &pci_resource_start(pdev, i),
                     (unsigned long long)pci_resource_len(pdev, i),
                     pci_resource_flags(pdev, i));
    }

    /* Read device status (e1000 register 0x0008) */
    status = ioread32(priv->bar0 + 0x0008);
    dev_info(&pdev->dev, "Device Status Register: 0x%08x\n", status);
    dev_info(&pdev->dev, "  Link Up: %s\n", (status & 0x02) ? "YES" : "NO");

    /* Enable bus mastering (required for DMA) */
    pci_set_master(pdev);

    pci_set_drvdata(pdev, priv);

    dev_info(&pdev->dev, "PCI demo driver probe complete\n");
    return 0;
}

static void my_pci_remove(struct pci_dev *pdev)
{
    pci_clear_master(pdev);
    dev_info(&pdev->dev, "PCI demo driver removed\n");
}

static const struct pci_device_id my_pci_ids[] = {
    { PCI_DEVICE(0x8086, 0x100e) },  /* Intel 82540EM (QEMU e1000) */
    { PCI_DEVICE(0x8086, 0x100f) },  /* Intel 82545EM */
    { PCI_DEVICE(0x8086, 0x10d3) },  /* Intel 82574L */
    { }
};
MODULE_DEVICE_TABLE(pci, my_pci_ids);

static struct pci_driver my_pci_driver = {
    .name     = "pci_demo",
    .id_table = my_pci_ids,
    .probe    = my_pci_probe,
    .remove   = my_pci_remove,
};
module_pci_driver(my_pci_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("PCI Device Driver Skeleton");
