#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

static struct platform_device *training_device;

static int __init training_device_init(void)
{
	int ret;

	training_device = platform_device_alloc("training-led", PLATFORM_DEVID_NONE);
	if (!training_device)
		return -ENOMEM;

	ret = platform_device_add(training_device);
	if (ret) {
		platform_device_put(training_device);
		return ret;
	}

	pr_info("training_device: Platform-Gerät registriert\n");
	return 0;
}

static void __exit training_device_exit(void)
{
	platform_device_unregister(training_device);
	pr_info("training_device: Platform-Gerät entfernt\n");
}

module_init(training_device_init);
module_exit(training_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Internals Training");
MODULE_DESCRIPTION("Künstliches Platform-Gerät für eine Übung");
