#define DEBUG

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

struct ap3216c_dev {
	unsigned char addr;				/* chip address - NOTE: 7bit*/
	struct i2c_adapter *adapter;	/* the adapter we sit on	*/
	struct device dev;				/* the device structure		*/

	dev_t devid;
	struct cdev cdev;
	struct class *class;
};
struct ap3216c_dev ap3216cDev;


int ap3216c_open(struct inode *inode, struct file *file)
{
	struct device dev = ap3216cDev.dev;

	dev_dbg(&dev, "%s\n", __func__);

	return 0;
}

int ap3216c_release(struct inode *inode, struct file *file)
{
	struct device dev = ap3216cDev.dev;

	dev_dbg(&dev, "%s\n", __func__);

	return 0;
}

ssize_t ap3216c_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	struct device dev = ap3216cDev.dev;
	unsigned char val;
	int ret;

	dev_dbg(&dev, "%s\n", __func__);

	val = 1;
	ret = copy_to_user(buf, &val, size);

	return ret;
}

const struct file_operations ap3216c_fops = {
	.owner    = THIS_MODULE,
	.open     = ap3216c_open,
	.release  = ap3216c_release,
	.read     = ap3216c_read,
};

int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct device dev = client->dev;

	ap3216cDev.addr = client->addr;
	ap3216cDev.adapter = client->adapter;
	ap3216cDev.dev = client->dev;

	dev_dbg(&dev, "%s: i2c addr 0x%x[7bits]\n", __func__, ap3216cDev.addr);

	alloc_chrdev_region(&ap3216cDev.devid, 0, 1, "ap3216cCharDev");
	cdev_init(&ap3216cDev.cdev, &ap3216c_fops);
	cdev_add(&ap3216cDev.cdev, ap3216cDev.devid, 1);
	ap3216cDev.class = class_create(THIS_MODULE, "ap3216cClass");
	device_create(ap3216cDev.class, NULL, ap3216cDev.devid, NULL, "ap3216cDev");

	return 0;
}

int ap3216c_remove(struct i2c_client *client)
{
	struct device dev = client->dev;

	dev_dbg(&dev, "%s\n", __func__);

	device_destroy(ap3216cDev.class, ap3216cDev.devid);
	class_destroy(ap3216cDev.class);
	cdev_del(&ap3216cDev.cdev);
	unregister_chrdev_region(ap3216cDev.devid, 1);

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
