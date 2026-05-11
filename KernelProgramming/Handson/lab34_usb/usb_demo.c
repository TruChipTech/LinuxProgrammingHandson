/*
 * usb_demo.c — USB Device Driver Demo
 *
 * Binds to a USB device by VID:PID, enumerates endpoints.
 * Use a USB flash drive or any USB device for testing.
 *
 * Usage: Change MY_USB_VENDOR/PRODUCT to match your device,
 *        or use module params.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/moduleparam.h>

static int vid = 0x0781;  /* SanDisk (example) */
static int pid = 0x5567;  /* Cruzer Blade (example) */
module_param(vid, int, 0444);
module_param(pid, int, 0444);
MODULE_PARM_DESC(vid, "USB Vendor ID (hex, default 0x0781)");
MODULE_PARM_DESC(pid, "USB Product ID (hex, default 0x5567)");

static int my_usb_probe(struct usb_interface *intf,
                         const struct usb_device_id *id)
{
    struct usb_device *udev = interface_to_usbdev(intf);
    struct usb_host_interface *iface_desc;
    int i;

    dev_info(&intf->dev, "=== USB Device Connected ===\n");
    dev_info(&intf->dev, "VID:PID = %04x:%04x\n",
             le16_to_cpu(udev->descriptor.idVendor),
             le16_to_cpu(udev->descriptor.idProduct));

    if (udev->manufacturer)
        dev_info(&intf->dev, "Manufacturer: %s\n", udev->manufacturer);
    if (udev->product)
        dev_info(&intf->dev, "Product: %s\n", udev->product);
    if (udev->serial)
        dev_info(&intf->dev, "Serial: %s\n", udev->serial);

    dev_info(&intf->dev, "Speed: %s\n",
             udev->speed == USB_SPEED_HIGH ? "High (480 Mbps)" :
             udev->speed == USB_SPEED_SUPER ? "Super (5 Gbps)" :
             udev->speed == USB_SPEED_FULL ? "Full (12 Mbps)" :
             "Other");

    dev_info(&intf->dev, "Class: %d, SubClass: %d, Protocol: %d\n",
             intf->cur_altsetting->desc.bInterfaceClass,
             intf->cur_altsetting->desc.bInterfaceSubClass,
             intf->cur_altsetting->desc.bInterfaceProtocol);

    /* Enumerate endpoints */
    iface_desc = intf->cur_altsetting;
    dev_info(&intf->dev, "Endpoints: %d\n",
             iface_desc->desc.bNumEndpoints);

    for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
        struct usb_endpoint_descriptor *ep = &iface_desc->endpoint[i].desc;

        dev_info(&intf->dev, "  EP[%d]: addr=0x%02x, type=%s, maxpkt=%d\n",
                 i,
                 ep->bEndpointAddress,
                 usb_endpoint_type(ep) == USB_ENDPOINT_XFER_BULK ? "BULK" :
                 usb_endpoint_type(ep) == USB_ENDPOINT_XFER_INT ? "INT" :
                 usb_endpoint_type(ep) == USB_ENDPOINT_XFER_ISOC ? "ISOC" :
                 "CTRL",
                 le16_to_cpu(ep->wMaxPacketSize));
    }

    return 0;
}

static void my_usb_disconnect(struct usb_interface *intf)
{
    dev_info(&intf->dev, "USB device disconnected\n");
}

/* Dynamic ID table — set via module params */
static struct usb_device_id my_usb_id[] = {
    { USB_DEVICE(0x0781, 0x5567) },  /* Will be overridden */
    { }
};

static struct usb_driver my_usb_driver = {
    .name       = "usb_demo",
    .probe      = my_usb_probe,
    .disconnect = my_usb_disconnect,
    .id_table   = my_usb_id,
};

static int __init usb_demo_init(void)
{
    /* Update ID table with module parameters */
    my_usb_id[0].idVendor = vid;
    my_usb_id[0].idProduct = pid;
    my_usb_id[0].match_flags = USB_DEVICE_ID_MATCH_DEVICE;

    pr_info("[usb_demo] Registering for VID:PID = %04x:%04x\n", vid, pid);
    return usb_register(&my_usb_driver);
}

static void __exit usb_demo_exit(void)
{
    usb_deregister(&my_usb_driver);
    pr_info("[usb_demo] Unregistered\n");
}

module_init(usb_demo_init);
module_exit(usb_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("USB Device Driver Demo");
