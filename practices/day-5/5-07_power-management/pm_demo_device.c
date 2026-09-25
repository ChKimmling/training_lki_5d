// SPDX-License-Identifier: GPL-2.0
/*
 * pm_demo_device.c - Trainingsbeispiel: künstliches Platform-Gerät
 * "pm-demo" als Gegenstück zu pm_demo_driver.ko.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

static struct platform_device *pm_demo_device;

static int __init pm_demo_device_init(void)
{
	int ret;

	pm_demo_device = platform_device_alloc("pm-demo", PLATFORM_DEVID_NONE);
	if (!pm_demo_device)
		return -ENOMEM;

	ret = platform_device_add(pm_demo_device);
	if (ret) {
		platform_device_put(pm_demo_device);
		return ret;
	}

	pr_info("pm_demo_device: Platform-Gerät registriert\n");
	return 0;
}

static void __exit pm_demo_device_exit(void)
{
	platform_device_unregister(pm_demo_device);
	pr_info("pm_demo_device: Platform-Gerät entfernt\n");
}

module_init(pm_demo_device_init);
module_exit(pm_demo_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Internals Training");
MODULE_DESCRIPTION("Künstliches Platform-Gerät für Übung 5.06 (Power Management)");
