#define DEBUG

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/input.h>

struct key_dev {
	struct platform_device *pdev;
	unsigned int key_irq;
	unsigned int value;

	struct input_dev *key_inputDev;
};
struct key_dev keyDev;


void work_func(struct work_struct *work)
{
	struct platform_device *pdev = keyDev.pdev;
	struct device dev = pdev->dev;
	struct input_dev *key_inputDev = keyDev.key_inputDev;

	dev_dbg(&dev, "%s: value = %d\n", __func__, keyDev.value);

	input_report_key(key_inputDev, KEY_0, keyDev.value);
	input_sync(key_inputDev);
}

DECLARE_WORK(key_irq_work, work_func);

static irqreturn_t key_irq_handler(int irq, void *dev)
{
	struct key_dev *p_keyDev = (struct key_dev *)dev;
	struct platform_device *pdev = p_keyDev->pdev;
	struct device p_dev = pdev->dev;

	dev_dbg(&p_dev, "%s: \n", __func__);

	p_keyDev->value = !p_keyDev->value;

	schedule_work(&key_irq_work);

	return IRQ_HANDLED;
}

int key_probe(struct platform_device *pdev)
{
	struct device dev = pdev->dev;
	struct device_node *np = dev.of_node;
	unsigned int key_irq;
	unsigned int ret;
	struct input_dev *key_inputDev;

	dev_dbg(&dev, "%s: \n", __func__);

	key_irq = irq_of_parse_and_map(np, 0);
	ret = request_irq(key_irq, key_irq_handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "keyIRQ", &keyDev);
	if(ret != 0) {
		dev_err(&dev, "request irq failed, ret %d\n", ret);
		return ret;
	}

	keyDev.pdev = pdev;
	keyDev.key_irq = key_irq;

	key_inputDev = input_allocate_device();
	key_inputDev->name = "key_inputDev";
	__set_bit(EV_KEY, key_inputDev->evbit);
	__set_bit(KEY_0,  key_inputDev->keybit);
	ret = input_register_device(key_inputDev);

	keyDev.key_inputDev = key_inputDev;

	return 0;
}

int key_remove(struct platform_device *pdev)
{
	struct device dev = pdev->dev;

	dev_dbg(&dev, "%s: \n", __func__);

	free_irq(keyDev.key_irq, &keyDev);

	input_unregister_device(keyDev.key_inputDev);
	input_free_device(keyDev.key_inputDev);

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
