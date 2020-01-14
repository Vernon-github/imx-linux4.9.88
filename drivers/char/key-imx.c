#define DEBUG

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

struct key_dev {
	struct platform_device *pdev;
	struct gpio_desc *key_gpio;

	dev_t devid;
	struct cdev cdev;
	struct class *class;
};
struct key_dev keyDev;

int key_open(struct inode *inode, struct file *file)
{
	struct platform_device *pdev = keyDev.pdev;
	struct device dev = pdev->dev;

	dev_dbg(&dev, "%s: \n", __func__);

	return 0;
}

int key_release(struct inode *inode, struct file *file)
{
	struct platform_device *pdev = keyDev.pdev;
	struct device dev = pdev->dev;

	dev_dbg(&dev, "%s: \n", __func__);

	return 0;
}

ssize_t key_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	struct platform_device *pdev = keyDev.pdev;
	struct device dev = pdev->dev;
	unsigned char status, ret;

    status = gpiod_get_value(keyDev.key_gpio);

	ret = copy_to_user(buf, &status, size);
	dev_dbg(&dev, "%s: status %d\n", __func__, status);

	return 0;
}

const struct file_operations key_fops = {
	.owner    = THIS_MODULE,
	.open     = key_open,
	.release  = key_release,
	.read    = key_read,
};

int key_probe(struct platform_device *pdev)
{
	struct device dev = pdev->dev;
	struct gpio_desc *key_gpio;

	dev_dbg(&dev, "%s: \n", __func__);

	key_gpio = gpiod_get(&dev, "key", GPIOD_IN);

	keyDev.pdev = pdev;
	keyDev.key_gpio = key_gpio;

	alloc_chrdev_region(&keyDev.devid, 0, 1, "keyCharDev");
	cdev_init(&keyDev.cdev, &key_fops);
	cdev_add(&keyDev.cdev, keyDev.devid, 1);
	keyDev.class = class_create(THIS_MODULE, "keyClass");
	device_create(keyDev.class, NULL, keyDev.devid, NULL, "keyDev");

	return 0;
}

int key_remove(struct platform_device *pdev)
{
	struct device dev = pdev->dev;

	dev_dbg(&dev, "%s: \n", __func__);

	gpiod_put(keyDev.key_gpio);

	device_destroy(keyDev.class, keyDev.devid);
	class_destroy(keyDev.class);
	cdev_del(&keyDev.cdev);
	unregister_chrdev_region(keyDev.devid, 1);

	return 0;
}

static const struct of_device_id of_key_match[] = {
	{ .compatible = "atk,key", },
	{},
};

MODULE_DEVICE_TABLE(of, of_key_match);

static struct platform_driver key_driver = {
	.probe  = key_probe,
	.remove = key_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "key",
		.of_match_table = of_key_match,
	},
};

module_platform_driver(key_driver);

MODULE_AUTHOR("lincheng Yang <2669889282@qq.com>");
MODULE_DESCRIPTION("key mode");
MODULE_LICENSE("GPL v2");
