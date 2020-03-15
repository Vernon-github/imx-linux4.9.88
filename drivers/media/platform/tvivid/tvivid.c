#define DEBUG

#include <linux/module.h>
#include <linux/platform_device.h>


int tvivid_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	dev_dbg(dev, "%s: \n", __func__);


	return 0;
}

int tvivid_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	dev_dbg(dev, "%s: \n", __func__);


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
