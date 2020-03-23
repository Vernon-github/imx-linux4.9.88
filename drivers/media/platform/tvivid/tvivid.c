#define DEBUG

#include <linux/module.h>
#include <linux/platform_device.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-device.h>
#include <media/videobuf2-v4l2.h>
#include <media/v4l2-ioctl.h>

struct tvivid_device {
	struct video_device vdev;
	struct v4l2_device v4l2_dev;
};
struct tvivid_device *tvivid;


int tvidioc_querycap(struct file *file, void *fh, struct v4l2_capability *cap)
{
	struct tvivid_device *tvivid_dev = video_drvdata(file);
	struct device *dev = &tvivid_dev->vdev.dev;

	dev_dbg(dev, "%s: \n", __func__);

	return 0;
}

int tvivid_enum_fmt_vid_cap(struct file *file, void *fh, struct v4l2_fmtdesc *f)
{
	struct tvivid_device *tvivid_dev = video_drvdata(file);
	struct device *dev = &tvivid_dev->vdev.dev;

	dev_dbg(dev, "%s: \n", __func__);

	return 0;
}

int tvivid_g_fmt_vid_cap(struct file *file, void *fh, struct v4l2_format *f)
{
	struct tvivid_device *tvivid_dev = video_drvdata(file);
	struct device *dev = &tvivid_dev->vdev.dev;

	dev_dbg(dev, "%s: \n", __func__);

	return 0;
}

int tvivid_s_fmt_vid_cap(struct file *file, void *fh, struct v4l2_format *f)
{
	struct tvivid_device *tvivid_dev = video_drvdata(file);
	struct device *dev = &tvivid_dev->vdev.dev;

	dev_dbg(dev, "%s: \n", __func__);

	return 0;
}

const struct v4l2_file_operations tvivid_fops = {
	.owner          = THIS_MODULE,
	.open           = v4l2_fh_open,
	.release        = v4l2_fh_release,
	.poll           = vb2_fop_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap           = vb2_fop_mmap,
};

const struct v4l2_ioctl_ops tvivid_ioctl_ops = {
	.vidioc_querycap         = tvidioc_querycap,

	.vidioc_enum_fmt_vid_cap = tvivid_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap    = tvivid_g_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap    = tvivid_s_fmt_vid_cap,

	.vidioc_reqbufs          = vb2_ioctl_reqbufs,
	.vidioc_querybuf         = vb2_ioctl_querybuf,
	.vidioc_qbuf             = vb2_ioctl_qbuf,
	.vidioc_dqbuf            = vb2_ioctl_dqbuf,
	.vidioc_streamon         = vb2_ioctl_streamon,
	.vidioc_streamoff        = vb2_ioctl_streamoff,
};

void tvivid_release(struct video_device *vdev)
{
	struct device *dev = &vdev->dev;

	dev_dbg(dev, "%s: \n", __func__);
}

int tvivid_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct video_device *vdev;
	int ret;

	dev_dbg(dev, "%s: \n", __func__);

	tvivid = kzalloc(sizeof(*tvivid), GFP_KERNEL);

	ret = v4l2_device_register(dev, &tvivid->v4l2_dev);

	vdev = &tvivid->vdev;
	vdev->fops = &tvivid_fops;
	vdev->ioctl_ops = &tvivid_ioctl_ops;
	vdev->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	vdev->release = tvivid_release;
	vdev->v4l2_dev = &tvivid->v4l2_dev;
	video_set_drvdata(vdev, tvivid);
	ret = video_register_device(vdev, VFL_TYPE_GRABBER, -1);

	return 0;
}

int tvivid_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	dev_dbg(dev, "%s: \n", __func__);

	video_unregister_device(&tvivid->vdev);
	v4l2_device_put(&tvivid->v4l2_dev);
	kfree(tvivid);

	return 0;
}

static const struct of_device_id of_tvivid_match[] = {
	{ .compatible = "test,vivid", },
	{},
};
MODULE_DEVICE_TABLE(of, of_tvivid_match);

static struct platform_driver tvivid_driver = {
	.probe  = tvivid_probe,
	.remove = tvivid_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "tvivid",
		.of_match_table = of_tvivid_match,
	},
};

module_platform_driver(tvivid_driver);

MODULE_AUTHOR("lincheng Yang <2669889282@qq.com>");
MODULE_DESCRIPTION("A Virtual Video Test Code For Learn.");
MODULE_LICENSE("GPL v2");
