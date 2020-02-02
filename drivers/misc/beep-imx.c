#define DEBUG

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>

struct beep_dev {
	struct platform_device *pdev;
	struct gpio_desc *beep_gpio;
};
struct beep_dev beepDev;

int beep_open(struct inode *inode, struct file *file)
{
	struct platform_device *pdev = beepDev.pdev;
	struct device dev = pdev->dev;

	dev_dbg(&dev, "%s: \n", __func__);

	return 0;
}

int beep_release(struct inode *inode, struct file *file)
{
	struct platform_device *pdev = beepDev.pdev;
	struct device dev = pdev->dev;

	dev_dbg(&dev, "%s: \n", __func__);

	return 0;
}

ssize_t beep_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	struct platform_device *pdev = beepDev.pdev;
	struct device dev = pdev->dev;
	unsigned char status, ret;

	ret = copy_from_user(&status, buf, size);
	dev_dbg(&dev, "%s: status %d\n", __func__, status);

	gpiod_set_value(beepDev.beep_gpio, status);

	return 0;
}

const struct file_operations beep_fops = {
	.owner    = THIS_MODULE,
	.open     = beep_open,
	.release  = beep_release,
	.write    = beep_write,
};

struct miscdevice beep_miscdevice = {
	.minor = 20,
	.name  = "beepDev",
	.fops  = &beep_fops,
};

int beep_probe(struct platform_device *pdev)
{
	struct device dev = pdev->dev;
	struct gpio_desc *beep_gpio;

	dev_dbg(&dev, "%s: \n", __func__);

	beep_gpio = gpiod_get(&dev, "beep", GPIOD_OUT_LOW);

	beepDev.pdev = pdev;
	beepDev.beep_gpio = beep_gpio;

	misc_register(&beep_miscdevice);

	return 0;
}

int beep_remove(struct platform_device *pdev)
{
	struct device dev = pdev->dev;

	dev_dbg(&dev, "%s: \n", __func__);

	gpiod_put(beepDev.beep_gpio);

	misc_deregister(&beep_miscdevice);

	return 0;
}

static const struct of_device_id of_beep_match[] = {
	{ .compatible = "atk,beep", },
	{},
};

MODULE_DEVICE_TABLE(of, of_beep_match);

static struct platform_driver beep_driver = {
	.probe  = beep_probe,
	.remove = beep_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "beep",
		.of_match_table = of_beep_match,
	},
};

module_platform_driver(beep_driver);

MODULE_AUTHOR("lincheng Yang <2669889282@qq.com>");
MODULE_DESCRIPTION("beep mode");
MODULE_LICENSE("GPL v2");
