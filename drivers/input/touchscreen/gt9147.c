#define DEBUG

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/gpio/consumer.h>

#define GT9147_ID_REGISTER 0x8140


struct gt9147_device {
    struct regmap *regmap;
    struct gpio_desc *reset_gpio;

};
struct gt9147_device gt9147Dev;

void gt9147_reset(struct i2c_client *client)
{
    struct device dev = client->dev;
    struct gpio_desc *reset_gpio;

    dev_dbg(&dev, "%s\n", __func__);
    reset_gpio = gpiod_get(&dev, "reset", GPIOD_OUT_LOW);

    gpiod_set_value(reset_gpio, true);
    msleep(10);
    // INT up pull in external, so i2c addr is 0x14
    msleep(1);
    gpiod_set_value(reset_gpio, false);
    msleep(5);

    gt9147Dev.reset_gpio = reset_gpio;
}

void gt9147_read_ID(struct i2c_client *client)
{
    struct device dev = client->dev;
    struct regmap *regmap = gt9147Dev.regmap;
    unsigned char buff[5];

    regmap_bulk_read(regmap, GT9147_ID_REGISTER, buff,sizeof(buff)-1);
    buff[4]='\0';
    dev_dbg(&dev, "%s: ID %s\n", __func__, buff);
}

int gt9147_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct device dev = client->dev;
    struct regmap_config config;

    dev_dbg(&dev, "%s: client addr 0x%x\n", __func__, client->addr);

    memset(&config, 0, sizeof(config));
    config.reg_bits = 16;
    config.val_bits = 8;
    gt9147Dev.regmap = devm_regmap_init_i2c(client, &config);

    gt9147_reset(client);

    gt9147_read_ID(client);

    return 0;
}

int gt9147_remove(struct i2c_client *client)
{
    gpiod_put(gt9147Dev.reset_gpio);

    return 0;
}

const struct i2c_device_id gt9147_id_table[] = {
	{ }
};

const struct of_device_id gt9147_of_match_table[] = {
    { .compatible = "goodix,gt9147" },
    { }
};

struct i2c_driver gt9147_i2c_driver = {
    .probe  = gt9147_probe,
    .remove = gt9147_remove,
    .id_table = gt9147_id_table,
    .driver = {
        .name  = "gt9147",
        .owner = THIS_MODULE,
        .of_match_table = gt9147_of_match_table,
    },
};

module_i2c_driver(gt9147_i2c_driver);

MODULE_AUTHOR("lincheng Yang <2669889282@qq.com>");
MODULE_DESCRIPTION("Goodix gt9147 touchscreen driver");
MODULE_LICENSE("GPL v2");
