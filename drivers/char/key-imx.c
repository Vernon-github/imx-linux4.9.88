#define DEBUG

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/poll.h>

struct key_dev {
	struct platform_device *pdev;
	unsigned int key_irq;
	unsigned char key_val;

	struct fasync_struct *fasync_queue;

	dev_t devid;
	struct cdev cdev;
	struct class *class;
};
struct key_dev keyDev;

DECLARE_WAIT_QUEUE_HEAD(key_irq_waitQueueHead);

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

	if(file->f_flags & O_NONBLOCK)
	{
		dev_dbg(&dev, "%s: nonblock IO mode\n", __func__);
	}
	else
	{
		dev_dbg(&dev, "%s: block IO mode\n", __func__);

		DECLARE_WAITQUEUE(wq, current);
		add_wait_queue(&key_irq_waitQueueHead, &wq);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule(); // sleep, wait wake up

		remove_wait_queue(&key_irq_waitQueueHead, &wq);
	}

	status = keyDev.key_val;
	keyDev.key_val = 0;

	ret = copy_to_user(buf, &status, size);
	dev_dbg(&dev, "%s: status %d\n", __func__, status);

	return 0;
}

unsigned int key_poll(struct file *file, struct poll_table_struct *wait)
{
	struct platform_device *pdev = keyDev.pdev;
	struct device dev = pdev->dev;

	dev_dbg(&dev, "%s: \n", __func__);

	poll_wait(file, &key_irq_waitQueueHead, wait);

	return keyDev.key_val? POLLIN : 0;
}

int key_fasync(int fd, struct file *file, int on)
{
	struct platform_device *pdev = keyDev.pdev;
	struct device dev = pdev->dev;

	dev_dbg(&dev, "%s: \n", __func__);

	return fasync_helper(fd, file, on, &keyDev.fasync_queue);
}

const struct file_operations key_fops = {
	.owner    = THIS_MODULE,
	.open     = key_open,
	.release  = key_release,
	.read    = key_read,
	.poll    = key_poll,
	.fasync  = key_fasync,
};

void work_func(struct work_struct *work)
{
	struct platform_device *pdev = keyDev.pdev;
	struct device dev = pdev->dev;

	dev_dbg(&dev, "%s: \n", __func__);

	keyDev.key_val=1;

	wake_up(&key_irq_waitQueueHead);

	kill_fasync(&keyDev.fasync_queue, SIGIO, POLL_IN);
}

DECLARE_WORK(key_irq_work, work_func);

static irqreturn_t key_irq_handler(int irq, void *dev)
{
	struct key_dev *p_keyDev = (struct key_dev *)dev;
	struct platform_device *pdev = p_keyDev->pdev;
	struct device p_dev = pdev->dev;

	dev_dbg(&p_dev, "%s: \n", __func__);

	schedule_work(&key_irq_work);

	return IRQ_HANDLED;
}

int key_probe(struct platform_device *pdev)
{
	struct device dev = pdev->dev;
	struct device_node *np = dev.of_node;
	unsigned int key_irq;
	unsigned int ret;

	dev_dbg(&dev, "%s: \n", __func__);

	key_irq = irq_of_parse_and_map(np, 0);
	ret = request_irq(key_irq, key_irq_handler, IRQF_TRIGGER_FALLING, "keyIRQ", &keyDev);
	if(ret != 0) {
		dev_err(&dev, "request irq failed, ret %d\n", ret);
		return ret;
	}

	keyDev.pdev = pdev;
	keyDev.key_irq = key_irq;

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

	free_irq(keyDev.key_irq, &keyDev);

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
