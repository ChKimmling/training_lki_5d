#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

static bool fail_probe;
module_param(fail_probe, bool, 0644);
MODULE_PARM_DESC(fail_probe, "Erzwingt einen Fehler in probe()");

struct training_data {
	unsigned int probe_count;
};

static int training_probe(struct platform_device *pdev)
{
	struct training_data *data;

	if (fail_probe)
		return dev_err_probe(&pdev->dev, -EIO,
				"training_driver: erzwungener Initialisierungsfehler\n");

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->probe_count = 1;
	platform_set_drvdata(pdev, data);

	dev_info(&pdev->dev, "training_driver: probe(): Gerät initialisiert\n");
	return 0;
}

static int training_remove(struct platform_device *pdev)
{
	struct training_data *data = platform_get_drvdata(pdev);

	if (data)
		dev_info(&pdev->dev,
			 "training_driver: remove(): Gerät entfernt, probe_count=%u\n",
			 data->probe_count);
	else
		dev_info(&pdev->dev,
			 "training_driver: remove(): Gerät entfernt\n");

	return 0;
}

static struct platform_driver training_driver = {
	.probe = training_probe,
	.remove = training_remove,
	.driver = {
		.name = "training-led",
	},
};

module_platform_driver(training_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Internals Training");
MODULE_DESCRIPTION("Platform-Treiber für das künstliche Trainingsgerät");
