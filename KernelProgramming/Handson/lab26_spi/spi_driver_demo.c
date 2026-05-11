/*
 * spi_driver_demo.c — SPI Client Driver Demo
 *
 * Demonstrates SPI communication (e.g., reading JEDEC ID from SPI flash).
 * For loopback testing, wire MOSI to MISO.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/spi/spi.h>
#include <linux/of.h>

struct my_spi_priv {
    struct spi_device *spi;
};

/* Send and receive data */
static int my_spi_xfer(struct spi_device *spi, u8 *tx, u8 *rx, int len)
{
    struct spi_transfer xfer = {
        .tx_buf = tx,
        .rx_buf = rx,
        .len    = len,
    };
    struct spi_message msg;

    spi_message_init(&msg);
    spi_message_add_tail(&xfer, &msg);
    return spi_sync(spi, &msg);
}

static int my_spi_probe(struct spi_device *spi)
{
    struct my_spi_priv *priv;
    u8 tx_buf[4] = { 0x9F, 0x00, 0x00, 0x00 };  /* JEDEC ID command */
    u8 rx_buf[4] = { 0 };
    int ret;

    dev_info(&spi->dev, "Probe: chip_select=%d, max_speed=%d Hz, mode=%d\n",
             spi_get_chipselect(spi, 0), spi->max_speed_hz, spi->mode);

    priv = devm_kzalloc(&spi->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->spi = spi;

    /* Set SPI mode */
    spi->mode = SPI_MODE_0;
    spi->bits_per_word = 8;
    ret = spi_setup(spi);
    if (ret) {
        dev_err(&spi->dev, "SPI setup failed: %d\n", ret);
        return ret;
    }

    /* Try to read JEDEC ID (works with SPI flash) */
    ret = my_spi_xfer(spi, tx_buf, rx_buf, 4);
    if (ret) {
        dev_warn(&spi->dev, "SPI transfer failed: %d\n", ret);
    } else {
        dev_info(&spi->dev, "Response: %02x %02x %02x %02x\n",
                 rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3]);
    }

    /* Simple loopback test */
    {
        u8 test_tx[] = "SPI_TEST";
        u8 test_rx[sizeof(test_tx)] = { 0 };

        ret = my_spi_xfer(spi, test_tx, test_rx, sizeof(test_tx));
        if (!ret) {
            dev_info(&spi->dev, "Loopback TX: %s\n", test_tx);
            dev_info(&spi->dev, "Loopback RX: %s %s\n", test_rx,
                     memcmp(test_tx, test_rx, sizeof(test_tx)) == 0 ?
                     "(MATCH!)" : "(no match — MOSI not wired to MISO?)");
        }
    }

    spi_set_drvdata(spi, priv);
    return 0;
}

static void my_spi_remove(struct spi_device *spi)
{
    dev_info(&spi->dev, "Remove\n");
}

static const struct spi_device_id my_spi_id[] = {
    { "my_spi_device", 0 },
    { }
};
MODULE_DEVICE_TABLE(spi, my_spi_id);

static const struct of_device_id my_spi_of_match[] = {
    { .compatible = "lab,my-spi-device" },
    { }
};
MODULE_DEVICE_TABLE(of, my_spi_of_match);

static struct spi_driver my_spi_driver = {
    .driver = {
        .name           = "my_spi_device",
        .of_match_table = my_spi_of_match,
    },
    .probe    = my_spi_probe,
    .remove   = my_spi_remove,
    .id_table = my_spi_id,
};
module_spi_driver(my_spi_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("SPI Client Driver Demo");
