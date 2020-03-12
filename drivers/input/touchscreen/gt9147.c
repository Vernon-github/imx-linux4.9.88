/* #define DEBUG */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/gpio/consumer.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/debugfs.h>

#define GT9147_ID_REGISTER  0x8140
#define GT9147_CTL_REGISTER 0x8040
#define GT9147_CFG_REGISTER 0x8047
#define GT9147_STA_REGISTER 0x814E
#define GT9147_DAT_REGISTER 0x814F

#define MAX_SUPPORT_POINTS 5

struct gt9147_device {
    struct i2c_client *client;
    struct regmap *regmap;
    struct gpio_desc *int_gpio;
    struct input_dev *inputdev;
    struct dentry *touchDir;
    unsigned int touch;
};
struct gt9147_device gt9147Dev;

const unsigned char GT9147_CT[]=
{
	0x48,0xe0,0x01,0x10,0x01,0x05,0x0d,0x00,0x01,0x08,
	0x28,0x05,0x50,0x32,0x03,0x05,0x00,0x00,0xff,0xff,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x89,0x28,0x0a,
	0x17,0x15,0x31,0x0d,0x00,0x00,0x02,0x9b,0x03,0x25,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x32,0x00,0x00,
	0x00,0x0f,0x94,0x94,0xc5,0x02,0x07,0x00,0x00,0x04,
	0x8d,0x13,0x00,0x5c,0x1e,0x00,0x3c,0x30,0x00,0x29,
	0x4c,0x00,0x1e,0x78,0x00,0x1e,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x08,0x0a,0x0c,0x0e,0x10,0x12,0x14,0x16,
	0x18,0x1a,0x00,0x00,0x00,0x00,0x1f,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0x00,0x02,0x04,0x05,0x06,0x08,0x0a,0x0c,
	0x0e,0x1d,0x1e,0x1f,0x20,0x22,0x24,0x28,0x29,0xff,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,
};

void gt9147_configure_register(struct device *dev, struct regmap *regmap)
{
    unsigned int configuer_version;
    unsigned char buf[2];
    unsigned int i;

    regmap_read(regmap, GT9147_CFG_REGISTER, &configuer_version);
    dev_dbg(dev, "%s: current configuer version is 0x%x\n", __func__, configuer_version);
    if(configuer_version < GT9147_CT[0])
    {
        dev_dbg(dev, "%s: update configuer\n", __func__);
        regmap_bulk_write(regmap, GT9147_CFG_REGISTER, GT9147_CT, sizeof(GT9147_CT));

        buf[0] = 0; // configuration information Check code
        buf[1] = 1; // Enable update
        for(i=0; i<sizeof(GT9147_CT); i++) {
            buf[0] += GT9147_CT[i];
        }
        buf[0] = (~buf[0]) +1;
        regmap_bulk_write(regmap, 0x80FF, buf, sizeof(buf));
    }
}

void gt9147_reset(struct i2c_client *client)
{
    struct device *dev = &client->dev;
    struct regmap *regmap = gt9147Dev.regmap;
    struct gpio_desc *reset_gpio;
    struct gpio_desc *int_gpio;

    dev_dbg(dev, "%s\n", __func__);

    reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
    int_gpio = devm_gpiod_get(dev, "int", GPIOD_OUT_LOW);

    // hardware reset
    gpiod_set_value(reset_gpio, true);
    msleep(11);
    gpiod_set_value(int_gpio, (client->addr == 0x5D));
    usleep_range(100, 200);
    gpiod_set_value(reset_gpio, false);
    msleep(6);
    gpiod_set_value(int_gpio, true);
    msleep(50);
    gpiod_direction_input(int_gpio);

    regmap_write(regmap, GT9147_CTL_REGISTER, 0x02); // software reset
    gt9147_configure_register(dev, regmap);
    mdelay(200);
    regmap_write(regmap, GT9147_CTL_REGISTER, 0x00); // goto read (x,y)

    gt9147Dev.int_gpio = int_gpio;
}

void gt9147_read_ID(struct i2c_client *client)
{
    struct device *dev = &client->dev;
    struct regmap *regmap = gt9147Dev.regmap;
    unsigned char ID[5];

    regmap_bulk_read(regmap, GT9147_ID_REGISTER, ID, sizeof(ID)-1);
    ID[4]='\0';
    dev_dbg(dev, "%s: ID %s\n", __func__, ID);
}

static irqreturn_t gt9147_irq_handler(int irq, void *dev)
{
	struct gt9147_device *p_gt9147Dev = (struct gt9147_device *)dev;
	struct i2c_client *client = p_gt9147Dev->client;
    struct regmap *regmap = p_gt9147Dev->regmap;
    unsigned int status = 0, touch_points = 0;
    struct input_dev *inputdev = p_gt9147Dev->inputdev;
    int slot[MAX_SUPPORT_POINTS], x[MAX_SUPPORT_POINTS], y[MAX_SUPPORT_POINTS], i, temp;
    unsigned char data[40];

    regmap_read(regmap, GT9147_STA_REGISTER, &status);
    touch_points = status & 0x0000000f;
    dev_dbg(&client->dev, "%s: status 0x%x, touch_points 0x%x, touch %d\n",
        __func__, status, touch_points, p_gt9147Dev->touch);

    if(!touch_points) {
        dev_dbg(&client->dev, "%s: number of touch points is 0\n", __func__);
        goto CLEAR_STATUS;
    }

    /*
     * 1. point number save to touch_points
     * 2. which point save to slot
     * 3. read position(x, y) save to x, y
     */
    regmap_bulk_read(regmap, GT9147_DAT_REGISTER, data, touch_points*8);
    for(i=0; i<touch_points; i++) {
        temp = i*8;
        slot[i] = data[temp];
        x[i] = data[temp+1] | (data[temp+2]<<8);
        y[i] = data[temp+3] | (data[temp+4]<<8);
        dev_dbg(&client->dev, "%s: slot %d, x %d, y %d\n",
            __func__, slot[i], x[i], y[i]);
    }

    for(i=0; i<touch_points; i++) {
        input_mt_slot(inputdev, slot[i]);
        input_mt_report_slot_state(inputdev, MT_TOOL_FINGER, p_gt9147Dev->touch);
        input_report_abs(inputdev, ABS_MT_POSITION_X, x[i]);
        input_report_abs(inputdev, ABS_MT_POSITION_Y, y[i]);
    }
    input_sync(inputdev);

CLEAR_STATUS:
    regmap_write(regmap, GT9147_STA_REGISTER, 0x0);

	return IRQ_HANDLED;
}

int gt9147_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct device *dev = &client->dev;
    struct regmap_config config;
    unsigned int irq, ret;
    struct input_dev *inputdev;

    dev_dbg(dev, "%s: client addr 0x%x\n", __func__, client->addr);

    memset(&config, 0, sizeof(config));
    config.reg_bits = 16;
    config.val_bits = 8;
    gt9147Dev.regmap = devm_regmap_init_i2c(client, &config);

    gt9147_reset(client);
    gt9147_read_ID(client);

    irq = gpiod_to_irq(gt9147Dev.int_gpio);
    ret = devm_request_threaded_irq(dev, irq, NULL, gt9147_irq_handler,
        IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "gt9147IRQ", &gt9147Dev);
    if(ret != 0) {
        dev_err(dev, "request irq failed, ret %d\n", ret);
        return ret;
    }

    inputdev = devm_input_allocate_device(dev);
    inputdev->name = "gt9147_inputdev";
    __set_bit(EV_ABS, inputdev->evbit);
    input_set_abs_params(inputdev, ABS_MT_POSITION_X, 0, 480, 0, 0);
    input_set_abs_params(inputdev, ABS_MT_POSITION_Y, 0, 272, 0, 0);
    input_mt_init_slots(inputdev, MAX_SUPPORT_POINTS, 0);
    ret = input_register_device(inputdev);

    gt9147Dev.touchDir = debugfs_create_dir("touchDir", NULL);
    debugfs_create_u32("touch", 0644, gt9147Dev.touchDir, &gt9147Dev.touch);

    gt9147Dev.client = client;
    gt9147Dev.inputdev = inputdev;

    return 0;
}

int gt9147_remove(struct i2c_client *client)
{
    input_unregister_device(gt9147Dev.inputdev);
    debugfs_remove_recursive(gt9147Dev.touchDir);

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
