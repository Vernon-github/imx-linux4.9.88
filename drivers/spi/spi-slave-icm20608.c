#define DEBUG

#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include "spi-slave-icm20608.h"

struct icm20608_dev {
    struct spi_device *spi;
    int cs_gpio;

    void (* read)(u8 , void *, int );
    void (* write)(u8 , void *, int );
};
struct icm20608_dev icm20608Dev;

void icm20608_read_regs(u8 reg, void *buf, int len)
{
    struct spi_device *spi = icm20608Dev.spi;
    struct device dev = spi->dev;
    struct spi_message m;
    struct spi_transfer *t;
    u8 spiReg = reg | 0x80;

    dev_dbg(&dev, "%s: \n", __func__);

    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
    gpio_set_value(icm20608Dev.cs_gpio, 0);

    t->tx_buf = &spiReg;
    t->len = 1;
    spi_message_init(&m);
    spi_message_add_tail(t, &m);
    spi_sync(spi, &m);

    t->rx_buf = buf;
    t->len = len;
    spi_message_init(&m);
    spi_message_add_tail(t, &m);
    spi_sync(spi, &m);

    gpio_set_value(icm20608Dev.cs_gpio, 1);
    kfree(t);
}

void icm20608_write_regs(u8 reg, void *buf, int len)
{
    struct spi_device *spi = icm20608Dev.spi;
    struct device dev = spi->dev;
    struct spi_message m;
    struct spi_transfer *t;
    u8 spiReg = reg & (~0x80);

    dev_dbg(&dev, "%s: \n", __func__);

    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
    gpio_set_value(icm20608Dev.cs_gpio, 0);

    t->tx_buf = &spiReg;
    t->len = 1;
    spi_message_init(&m);
    spi_message_add_tail(t, &m);
    spi_sync(spi, &m);

    t->tx_buf = buf;
    t->len = len;
    spi_message_init(&m);
    spi_message_add_tail(t, &m);
    spi_sync(spi, &m);

    gpio_set_value(icm20608Dev.cs_gpio, 1);
    kfree(t);
}

int	icm20608_probe(struct spi_device *spi)
{
    struct device dev = spi->dev;
    struct device_node *np = dev.of_node;
    int cs_gpio;

    dev_dbg(&dev, "%s: \n", __func__);

    cs_gpio = of_get_named_gpio(np->parent, "cs-gpio", 0);
    gpio_direction_output(cs_gpio, 1);

    spi->mode = SPI_MODE_0; /*MODE0，CPOL=0，CPHA=0*/
    spi_setup(spi);

    icm20608Dev.spi = spi;
    icm20608Dev.cs_gpio = cs_gpio;
    icm20608Dev.read = icm20608_read_regs;
    icm20608Dev.write = icm20608_write_regs;

    return 0;
}

int	icm20608_remove(struct spi_device *spi)
{
    struct device dev = spi->dev;

    dev_dbg(&dev, "%s: \n", __func__);

    return 0;
}

const struct of_device_id icm20608_of_match_table[] = {
    { .compatible = "alientek,icm20608", },
};

const struct spi_device_id icm20608_id_table[] = {
    { .name = "alientek,icm20608", },
};

struct spi_driver icm20608_sdrv = {
    .probe  = icm20608_probe,
    .remove = icm20608_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "icm20608",
        .of_match_table = icm20608_of_match_table,
    },
    .id_table = icm20608_id_table,
};

static int __init icm20608_init(void)
{
    spi_register_driver(&icm20608_sdrv);

    return 0;
}

static void __exit icm20608_exit(void)
{
    spi_unregister_driver(&icm20608_sdrv);
}

module_init(icm20608_init);
module_exit(icm20608_exit);

MODULE_AUTHOR("lincheng Yang <2669889282@qq.com>");
MODULE_DESCRIPTION("SPI slave device ICM20608");
MODULE_LICENSE("GPL v2");