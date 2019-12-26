#define DEBUG

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

struct led_dev {
	struct platform_device *pdev;
	struct gpio_desc *led_gpio;

	dev_t devid;
	struct cdev cdev;
	struct class *class;

};
struct led_dev ledDev;

int led_open(struct inode *inode, struct file *file)
{
	struct platform_device *pdev = ledDev.pdev;
	struct device dev = pdev->dev;

	dev_dbg(&dev, "%s: \n", __func__);

	return 0;
}

int led_release(struct inode *inode, struct file *file)
{
	struct platform_device *pdev = ledDev.pdev;
	struct device dev = pdev->dev;

	dev_dbg(&dev, "%s: \n", __func__);

	return 0;
}

ssize_t led_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	struct platform_device *pdev = ledDev.pdev;
	struct device dev = pdev->dev;
	unsigned char status, ret;

	ret = copy_from_user(&status, buf, size);
	dev_dbg(&dev, "%s: status %d\n", __func__, status);

	gpiod_set_value(ledDev.led_gpio, status);

	return 0;
}

const struct file_operations led_fops = {
	.owner    = THIS_MODULE,
	.open     = led_open,
	.release  = led_release,
	.write    = led_write,
};

int led_probe(struct platform_device *pdev)
{
	struct device dev = pdev->dev;
	struct gpio_desc *led_gpio;

	dev_dbg(&dev, "%s: \n", __func__);

	led_gpio = gpiod_get(&dev, "led", GPIOD_OUT_LOW);

	ledDev.pdev = pdev;
	ledDev.led_gpio = led_gpio;

	alloc_chrdev_region(&ledDev.devid, 0, 1, "ledCharDev");
	cdev_init(&ledDev.cdev, &led_fops);
	cdev_add(&ledDev.cdev, ledDev.devid, 1);
	ledDev.class = class_create(THIS_MODULE, "ledClass");
	device_create(ledDev.class, NULL, ledDev.devid, NULL, "ledDev");

	return 0;
}

int led_remove(struct platform_device *pdev)
{
	struct device dev = pdev->dev;

	dev_dbg(&dev, "%s: \n", __func__);

	gpiod_put(ledDev.led_gpio);

	device_destroy(ledDev.class, ledDev.devid);
	class_destroy(ledDev.class);
	cdev_del(&ledDev.cdev);
	unregister_chrdev_region(ledDev.devid, 1);

	return 0;
}

static const struct of_device_id of_led_match[] = {
	{ .compatible = "atk,led", },
	{},
};

MODULE_DEVICE_TABLE(of, of_led_match);

static struct platform_driver led_driver = {
	.probe  = led_probe,
	.remove = led_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "led",
		.of_match_table = of_led_match,
	},
};

module_platform_driver(led_driver);

MODULE_AUTHOR("lincheng Yang <2669889282@qq.com>");
MODULE_DESCRIPTION("led mode");
MODULE_LICENSE("GPL v2");
