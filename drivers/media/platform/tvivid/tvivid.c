#define DEBUG

#include <linux/module.h>
#include <linux/platform_device.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-device.h>
#include <media/videobuf2-v4l2.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-vmalloc.h>
#include <linux/delay.h>

struct tvivid_device {
	struct video_device vdev;
	struct v4l2_device v4l2_dev;
	struct vb2_queue queue;
	struct mutex mutex;
	struct list_head list;
	struct task_struct *kthread;

	unsigned int width;
	unsigned int height;
	unsigned int pixelformat;

	unsigned int bytesperline;
	unsigned int sizeimage;
};
struct tvivid_device *tvivid;

struct tvivid_buffer {
	/* common v4l buffer stuff -- must be first */
	struct vb2_v4l2_buffer vb;
	struct list_head list;
};

struct tvivid_fmt {
	u32	pixelformat;
	u32	bit_depth;
};

struct tvivid_fmt tvivid_formats[] = {
	{
		.pixelformat = V4L2_PIX_FMT_RGB565, /* gggbbbbb rrrrrggg */
		.bit_depth   = 16,
	},
};

int tvivid_queue_setup(struct vb2_queue *q,
		unsigned int *num_buffers, unsigned int *num_planes,
		unsigned int sizes[], struct device *alloc_devs[])
{
	struct tvivid_device *tvivid_dev = vb2_get_drv_priv(q);
	struct device *dev = &tvivid_dev->vdev.dev;

	dev_dbg(dev, "%s: num_buffers %d, sizeimage %d\n",
		__func__, *num_buffers, tvivid_dev->sizeimage);

	sizes[0] = tvivid_dev->sizeimage;
	*num_planes = 1;

	return 0;
}

int tvivid_buf_prepare(struct vb2_buffer *vb)
{
	struct tvivid_device *tvivid_dev = vb2_get_drv_priv(vb->vb2_queue);
	struct device *dev = &tvivid_dev->vdev.dev;

	dev_dbg(dev, "%s: \n", __func__);

	vb2_set_plane_payload(vb, 0, tvivid_dev->sizeimage);

	return 0;
}

void tvivid_buf_queue(struct vb2_buffer *vb)
{
	struct tvivid_device *tvivid_dev = vb2_get_drv_priv(vb->vb2_queue);
	struct device *dev = &tvivid_dev->vdev.dev;
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct tvivid_buffer *buf = container_of(vbuf, struct tvivid_buffer, vb);

	dev_dbg(dev, "%s: \n", __func__);

	list_add_tail(&buf->list, &tvivid_dev->list);
}

static int tvivid_kthread(void *data)
{
	struct tvivid_device *tvivid_dev = data;
	struct tvivid_buffer *buf = NULL;
	void *vbuf;

	for (;;) {
		if (kthread_should_stop())
			break;

		buf = list_entry(tvivid_dev->list.next, struct tvivid_buffer, list);
		list_del(&buf->list);

		vbuf = vb2_plane_vaddr(&buf->vb.vb2_buf, 0);
		memset(vbuf, 0x88, tvivid_dev->sizeimage);

		vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
		msleep(100);
	}

	return 0;
}

int tvivid_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct tvivid_device *tvivid_dev = vb2_get_drv_priv(q);
	struct device *dev = &tvivid_dev->vdev.dev;

	dev_dbg(dev, "%s: \n", __func__);

	tvivid_dev->kthread = kthread_run(tvivid_kthread, tvivid_dev, "tvivid-kthread");

	return 0;
}

void tvivid_stop_streaming(struct vb2_queue *q)
{
	struct tvivid_device *tvivid_dev = vb2_get_drv_priv(q);
	struct device *dev = &tvivid_dev->vdev.dev;

	dev_dbg(dev, "%s: \n", __func__);

	while (!list_empty(&tvivid_dev->list)) {
		struct tvivid_buffer *buf;

		buf = list_entry(tvivid_dev->list.next,
					struct tvivid_buffer, list);
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
	}

	kthread_stop(tvivid_dev->kthread);
}

const struct vb2_ops tvivid_queue_ops = {
	.queue_setup     = tvivid_queue_setup,
	.buf_prepare     = tvivid_buf_prepare,
	.buf_queue       = tvivid_buf_queue,
	.start_streaming = tvivid_start_streaming,
	.stop_streaming  = tvivid_stop_streaming,
};

int tvidioc_querycap(struct file *file, void *fh, struct v4l2_capability *cap)
{
	struct tvivid_device *tvivid_dev = video_drvdata(file);
	struct device *dev = &tvivid_dev->vdev.dev;

	dev_dbg(dev, "%s: \n", __func__);

	strcpy(cap->driver, "tvivid_driver");
	strcpy(cap->card, "tvivid_card");
	strcpy(cap->bus_info, "tvivid_bus_info");

	return 0;
}

int tvivid_enum_fmt_vid_cap(struct file *file, void *fh, struct v4l2_fmtdesc *f)
{
	struct tvivid_device *tvivid_dev = video_drvdata(file);
	struct device *dev = &tvivid_dev->vdev.dev;
	struct tvivid_fmt *fmt;

	dev_dbg(dev, "%s: index %d\n", __func__, f->index);

	if (f->index >= ARRAY_SIZE(tvivid_formats))
		return -EINVAL;

	fmt = &tvivid_formats[f->index];
	f->pixelformat = fmt->pixelformat;

	return 0;
}

int tvivid_g_fmt_vid_cap(struct file *file, void *fh, struct v4l2_format *f)
{
	struct tvivid_device *tvivid_dev = video_drvdata(file);
	struct device *dev = &tvivid_dev->vdev.dev;
	struct v4l2_pix_format pix = f->fmt.pix;

	dev_dbg(dev, "%s: \n", __func__);

	pix.width = tvivid_dev->width;
	pix.height = tvivid_dev->height;
	pix.pixelformat = tvivid_dev->pixelformat;

	pix.bytesperline = tvivid_dev->bytesperline;
	pix.sizeimage = tvivid_dev->sizeimage;

	return 0;
}

int tvivid_s_fmt_vid_cap(struct file *file, void *fh, struct v4l2_format *f)
{
	struct tvivid_device *tvivid_dev = video_drvdata(file);
	struct device *dev = &tvivid_dev->vdev.dev;
	struct v4l2_pix_format pix = f->fmt.pix;
	int i;

	dev_dbg(dev, "%s: width %d, height %d, bytesperline %d, sizeimage %d\n",
		__func__, pix.width, pix.height, pix.bytesperline, pix.sizeimage);

	tvivid_dev->width = pix.width;
	tvivid_dev->height = pix.height;
	tvivid_dev->pixelformat = pix.pixelformat;

	for(i=0; i<ARRAY_SIZE(tvivid_formats); i++) {
		if(pix.pixelformat == tvivid_formats[i].pixelformat)
			break;
	}
	tvivid_dev->bytesperline = tvivid_dev->width * (tvivid_formats[i].bit_depth >> 3);
	tvivid_dev->sizeimage = tvivid_dev->bytesperline * tvivid_dev->height;

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
	struct vb2_queue *queue;
	int ret;

	dev_dbg(dev, "%s: \n", __func__);

	tvivid = kzalloc(sizeof(*tvivid), GFP_KERNEL);

	ret = v4l2_device_register(dev, &tvivid->v4l2_dev);

	INIT_LIST_HEAD(&tvivid->list);
	mutex_init(&tvivid->mutex);

	queue = &tvivid->queue;
	queue->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	queue->io_modes = VB2_MMAP;
	queue->ops = &tvivid_queue_ops;
	queue->mem_ops = &vb2_vmalloc_memops;
	queue->buf_struct_size = sizeof(struct tvivid_buffer);
	queue->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	queue->lock = &tvivid->mutex;
	queue->drv_priv = tvivid;
	ret = vb2_queue_init(queue);

	vdev = &tvivid->vdev;
	vdev->fops = &tvivid_fops;
	vdev->ioctl_ops = &tvivid_ioctl_ops;
	vdev->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	vdev->release = tvivid_release;
	vdev->v4l2_dev = &tvivid->v4l2_dev;
	vdev->queue = queue;
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
