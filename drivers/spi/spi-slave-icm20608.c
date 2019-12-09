#define DEBUG

#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include "spi-slave-icm20608.h"

struct icm20608_dev {
    struct spi_device *spi;
    int cs_gpio;

    dev_t devid;
    struct cdev cdev;
    struct class *class;

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

void icm20608_read_onereg(u8 reg, u8 *val)
{
    icm20608_read_regs(reg, val, 1);
}

void icm20608_write_onereg(u8 reg, u8 val)
{
    icm20608_write_regs(reg, &val, 1);
}

int icm20608_open(struct inode *inode, struct file *file)
{
    struct spi_device *spi = icm20608Dev.spi;
    struct device dev = spi->dev;
    u8 id;

    dev_dbg(&dev, "%s: \n", __func__);

    icm20608_write_onereg(ICM20_PWR_MGMT_1, 0x80);
    mdelay(50);
    icm20608_write_onereg(ICM20_PWR_MGMT_1, 0x01);
    mdelay(50);

    icm20608_read_onereg(ICM20_WHO_AM_I, &id);
    dev_info(&dev, "ICM20608 ID = %#X\r\n", id);

    icm20608_write_onereg(ICM20_SMPLRT_DIV,    0x00); /* 输出速率是内部采样率      */
    icm20608_write_onereg(ICM20_GYRO_CONFIG,   0x18); /* 陀螺仪±2000dps量程      */
    icm20608_write_onereg(ICM20_ACCEL_CONFIG,  0x18); /* 加速度计±16G量程         */
    icm20608_write_onereg(ICM20_CONFIG,        0x04); /* 陀螺仪低通滤波BW=20Hz    */
    icm20608_write_onereg(ICM20_ACCEL_CONFIG2, 0x04); /* 加速度计低通滤波BW=21.2Hz */
    icm20608_write_onereg(ICM20_PWR_MGMT_2,    0x00); /* 打开加速度计和陀螺仪所有轴  */
    icm20608_write_onereg(ICM20_LP_MODE_CFG,   0x00); /* 关闭低功耗               */
    icm20608_write_onereg(ICM20_FIFO_EN,       0x00); /* 关闭FIFO                */

    return 0;
}

int icm20608_release(struct inode *inode, struct file *file)
{
    struct spi_device *spi = icm20608Dev.spi;
    struct device dev = spi->dev;

    dev_dbg(&dev, "%s: \n", __func__);

    return 0;
}

ssize_t icm20608_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
    struct spi_device *spi = icm20608Dev.spi;
    struct device dev = spi->dev;
    unsigned char data[14];
    unsigned short accel_x_adc, accel_y_adc, accel_z_adc, temp_adc;
    unsigned short gyro_x_adc, gyro_y_adc, gyro_z_adc;
    unsigned short accel_temp_gyro[7];
    int ret;

    dev_dbg(&dev, "%s: \n", __func__);

    icm20608_read_regs(ICM20_ACCEL_XOUT_H, data, sizeof(data));
    accel_x_adc = (signed short)((data[0] << 8) | data[1]);
    accel_y_adc = (signed short)((data[2] << 8) | data[3]);
    accel_z_adc = (signed short)((data[4] << 8) | data[5]);
    temp_adc    = (signed short)((data[6] << 8) | data[7]);
    gyro_x_adc  = (signed short)((data[8] << 8) | data[9]);
    gyro_y_adc  = (signed short)((data[10] << 8) | data[11]);
    gyro_z_adc  = (signed short)((data[12] << 8) | data[13]);

    accel_temp_gyro[0] = accel_x_adc;
    accel_temp_gyro[1] = accel_y_adc;
    accel_temp_gyro[2] = accel_z_adc;
    accel_temp_gyro[3] = temp_adc;
    accel_temp_gyro[4] = gyro_x_adc;
    accel_temp_gyro[5] = gyro_y_adc;
    accel_temp_gyro[6] = gyro_z_adc;

    ret = copy_to_user(buf, accel_temp_gyro, size);

    return 0;
}

const struct file_operations icm20608_fops = {
    .owner    = THIS_MODULE,
    .open     = icm20608_open,
    .release  = icm20608_release,
    .read     = icm20608_read,
};

int	icm20608_probe(struct spi_device *spi)
{
    struct device dev = spi->dev;
    struct device_node *np = dev.of_node;
    unsigned int cs_gpio;

    dev_dbg(&dev, "%s: \n", __func__);

    cs_gpio = of_get_named_gpio(np->parent, "cs-gpio", 0);
    gpio_direction_output(cs_gpio, 1);

    spi->mode = SPI_MODE_0; /*MODE0，CPOL=0，CPHA=0*/
    spi_setup(spi);

    icm20608Dev.spi = spi;
    icm20608Dev.cs_gpio = cs_gpio;
    icm20608Dev.read = icm20608_read_regs;
    icm20608Dev.write = icm20608_write_regs;

    alloc_chrdev_region(&icm20608Dev.devid, 0, 1, "icm20608CharDev");
    cdev_init(&icm20608Dev.cdev, &icm20608_fops);
    cdev_add(&icm20608Dev.cdev, icm20608Dev.devid, 1);
    icm20608Dev.class = class_create(THIS_MODULE, "icm20608Class");
    device_create(icm20608Dev.class, NULL, icm20608Dev.devid, NULL, "icm20608Dev");

    return 0;
}

int	icm20608_remove(struct spi_device *spi)
{
    struct device dev = spi->dev;

    dev_dbg(&dev, "%s: \n", __func__);

    device_destroy(icm20608Dev.class, icm20608Dev.devid);
    class_destroy(icm20608Dev.class);
    cdev_del(&icm20608Dev.cdev);
    unregister_chrdev_region(icm20608Dev.devid, 1);

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