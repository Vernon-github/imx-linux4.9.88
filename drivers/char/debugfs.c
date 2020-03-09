#define DEBUG

#include <linux/module.h>
#include <linux/debugfs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

struct dentry *test;
struct dentry *sub_test;
unsigned char c = 2;
unsigned char data = 'd';

int debugfs_open(struct inode *inode, struct file *file)
{
    pr_debug("%s\n", __func__);

    return 0;
}

int debugfs_release(struct inode *inode, struct file *file)
{
    pr_debug("%s\n", __func__);

    return 0;
}

ssize_t debugfs_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
    unsigned char ret, kbuf[3];

    kbuf[0] = data;
    kbuf[1] = '\n';
    kbuf[2] = '\0';
    ret = simple_read_from_buffer(buf, size, offset, kbuf, sizeof(kbuf));
    pr_debug("%s: data %c, kbuf %s, size %d, ret %d\n", __func__, data, kbuf, size, ret);

    return ret;
}

ssize_t debugfs_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    unsigned char ret, *kbuf;

    kbuf = kmalloc(size, GFP_KERNEL);

	ret = copy_from_user(kbuf, buf, size);
    kbuf[size-1] = '\0';

    data = kbuf[0];
    pr_debug("%s: data %c, kbuf %s, size %d, ret %d\n", __func__, data, kbuf, size, ret);

    kfree(kbuf);

    return size;
}

const struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = debugfs_open,
    .release = debugfs_release,
    .read    = debugfs_read,
    .write   = debugfs_write,
};

static int __init debugfs_init(void)
{
    pr_debug("%s\n", __func__);

    test = debugfs_create_dir("test", NULL);
    sub_test = debugfs_create_dir("sub-test", test);

    debugfs_create_u8("c", 0644, test, &c);
    debugfs_create_file("data", 0644, sub_test, NULL, &fops);

    return 0;
}

static void __exit debugfs_exit(void)
{
    pr_debug("%s\n", __func__);

    debugfs_remove_recursive(test);
}

module_init(debugfs_init);
module_exit(debugfs_exit);

MODULE_AUTHOR("lincheng Yang <2669889282@qq.com>");
MODULE_DESCRIPTION("debugfs driver");
MODULE_LICENSE("GPL v2");