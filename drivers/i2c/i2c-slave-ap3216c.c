#define DEBUG

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/device.h>

struct ap3216c_dev {
	unsigned char addr;				/* chip address - NOTE: 7bit*/
	struct i2c_adapter *adapter;	/* the adapter we sit on	*/
	struct device dev;				/* the device structure		*/
};
struct ap3216c_dev ap3216cDev;

int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct device dev = client->dev;

	ap3216cDev.addr = client->addr;
	ap3216cDev.adapter = client->adapter;
	ap3216cDev.dev = client->dev;

	dev_dbg(&dev, "%s: i2c addr 0x%x[7bits]\n", __func__, ap3216cDev.addr);

	return 0;
}

int ap3216c_remove(struct i2c_client *client)
{
	struct device dev = client->dev;

	dev_dbg(&dev, "%s\n", __func__);

	return 0;
}

const struct of_device_id ap3216c_of_match_table[] = {
	{ .compatible = "lsc,ap3216c" },
	{}
};

const struct i2c_device_id ap3216c_id_table[] = {
	//{ .name = "lsc,ap3216c" },
	{}
};

struct i2c_driver ap3216c_i2c_driver = {
	.probe = ap3216c_probe,
	.remove = ap3216c_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "ap3216c",
		.of_match_table = ap3216c_of_match_table,
	},
	/* must add .id_table, otherwise .probe function can not be executed */
	.id_table = ap3216c_id_table,
};

static int __init ap3216c_init(void)
{
	i2c_add_driver(&ap3216c_i2c_driver);
	return 0;
}

static void __exit ap3216c_exit(void)
{
	i2c_del_driver(&ap3216c_i2c_driver);
}

module_init(ap3216c_init);
module_exit(ap3216c_exit);

MODULE_AUTHOR("lincheng Yang <2669889282@qq.com>");
MODULE_DESCRIPTION("I2C slave mode AP3216C");
MODULE_LICENSE("GPL v2");
