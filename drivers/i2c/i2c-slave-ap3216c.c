#define DEBUG

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/delay.h>

/* AP3316C寄存器 */
#define AP3216C_SYSTEMCONG	0x00	/* 配置寄存器     */
#define AP3216C_IRDATALOW	0x0A	/* IR数据低字节   */
#define AP3216C_IRDATAHIGH	0x0B	/* IR数据高字节   */
#define AP3216C_ALSDATALOW	0x0C	/* ALS数据低字节  */
#define AP3216C_ALSDATAHIGH	0X0D	/* ALS数据高字节  */
#define AP3216C_PSDATALOW	0X0E	/* PS数据低字节   */
#define AP3216C_PSDATAHIGH	0X0F	/* PS数据高字节   */

struct ap3216c_dev {
	unsigned char addr;				/* chip address - NOTE: 7bit*/
	struct i2c_adapter *adapter;	/* the adapter we sit on	*/
	struct device dev;				/* the device structure		*/

	dev_t devid;
	struct cdev cdev;
	struct class *class;
};
struct ap3216c_dev ap3216cDev;

void ap3216c_write_regs(u8 reg, u8 *buf, u8 len)
{
	struct i2c_msg msg;
	u8 buffer[256];

	msg.addr = ap3216cDev.addr;
	msg.flags = 0;
	msg.len = len + 1;

	buffer[0] = reg;
	memcpy(&buffer[1], buf, len);
	msg.buf = buffer;

	i2c_transfer(ap3216cDev.adapter, &msg, 1);
}

void ap3216c_read_regs(u8 reg, u8 *buf, u8 len)
{
	struct i2c_msg msg[2];

	msg[0].addr = ap3216cDev.addr;
	msg[0].flags = 0;
	msg[0].buf = &reg;
	msg[0].len = 1;

	msg[1].addr = ap3216cDev.addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = buf;
	msg[1].len = len;

	i2c_transfer(ap3216cDev.adapter, msg, 2);
}

int ap3216c_open(struct inode *inode, struct file *file)
{
	struct device dev = ap3216cDev.dev;
	u8 val;

	dev_dbg(&dev, "%s\n", __func__);

	val = 0x04;
	ap3216c_write_regs(AP3216C_SYSTEMCONG, &val, 1); /* 复位AP3216C */
	mdelay(50);                                      /* AP3216C复位最少10ms */
	val = 0x03;
	ap3216c_write_regs(AP3216C_SYSTEMCONG, &val, 1); /* 开启ALS、PS+IR */
	mdelay(120);

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
	unsigned char buffer[6];
	unsigned short ir, als, ps, ir_als_ps[3];
	int i, ret;

	dev_dbg(&dev, "%s\n", __func__);

	for(i=0; i<sizeof(buffer); i++) {
		ap3216c_read_regs(AP3216C_IRDATALOW + i, &buffer[i], 1);
	}

	ir = (buffer[1] << 2) | (buffer[0] & 0X03);
	als = (buffer[3] << 8) | buffer[2];
	ps = ((buffer[5] & 0X3F) << 4) | (buffer[4] & 0X0F);
	dev_dbg(&dev, "%s: ir 0x%x, als 0x%x, ps 0x%x\n", __func__, ir, als, ps);

	ir_als_ps[0] = ir;
	ir_als_ps[1] = als;
	ir_als_ps[2] = ps;

	ret = copy_to_user(buf, ir_als_ps, size);

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
