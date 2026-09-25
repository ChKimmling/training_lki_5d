// SPDX-License-Identifier: GPL-2.0
/*
 * pm_demo_driver.c - Trainingsbeispiel: Platform-Treiber mit Runtime-PM,
 * Autosuspend, System-Sleep-Wiederverwendung (force_suspend/force_resume)
 * und Wakeup-Integration. "shadow_reg" simuliert ein Hardware-Register,
 * das beim Suspend seinen Wert verliert und beim Resume restauriert wird.
 */

#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/pm_wakeup.h>
#include <linux/sysfs.h>

#define PM_DEMO_AUTOSUSPEND_DELAY_MS 2000
#define PM_DEMO_REG_RESET_VALUE      0xCAFE

struct pm_demo_data {
	u32 shadow_reg;
	unsigned int access_count;
};

static int pm_demo_runtime_suspend(struct device *dev)
{
	struct pm_demo_data *data = dev_get_drvdata(dev);

	/* Simuliert: Clock/Regulator abschalten, Register geht verloren */
	dev_info(dev, "runtime_suspend: Ressourcen abschalten (letztes shadow_reg=0x%x)\n",
		 data->shadow_reg);
	data->shadow_reg = 0;
	return 0;
}

static int pm_demo_runtime_resume(struct device *dev)
{
	struct pm_demo_data *data = dev_get_drvdata(dev);

	/* Simuliert: Clock/Regulator einschalten, Register restaurieren */
	data->shadow_reg = PM_DEMO_REG_RESET_VALUE;
	dev_info(dev, "runtime_resume: Ressourcen einschalten (shadow_reg=0x%x)\n",
		 data->shadow_reg);
	return 0;
}

static const struct dev_pm_ops pm_demo_pm_ops = {
	.runtime_suspend = pm_demo_runtime_suspend,
	.runtime_resume  = pm_demo_runtime_resume,
	.suspend = pm_runtime_force_suspend,
	.resume  = pm_runtime_force_resume,
};

static ssize_t access_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct pm_demo_data *data = dev_get_drvdata(dev);
	int ret;

	ret = pm_runtime_resume_and_get(dev);
	if (ret < 0)
		return ret;

	data->access_count++;
	dev_info(dev, "access: Zugriff #%u bei aktivem Gerät (shadow_reg=0x%x)\n",
		 data->access_count, data->shadow_reg);

	pm_runtime_mark_last_busy(dev);
	pm_runtime_put_autosuspend(dev);

	return count;
}
static DEVICE_ATTR_WO(access);

static ssize_t access_count_show(struct device *dev, struct device_attribute *attr,
				  char *buf)
{
	struct pm_demo_data *data = dev_get_drvdata(dev);

	return sysfs_emit(buf, "%u\n", data->access_count);
}
static DEVICE_ATTR_RO(access_count);

static ssize_t simulate_wakeup_store(struct device *dev, struct device_attribute *attr,
				      const char *buf, size_t count)
{
	/* Simuliert ein IRQ-Ereignis, das einen konkurrierenden Suspend verhindert */
	pm_wakeup_dev_event(dev, 0, true);
	dev_info(dev, "simulate_wakeup: Wakeup-Event ausgelöst\n");
	return count;
}
static DEVICE_ATTR_WO(simulate_wakeup);

static struct attribute *pm_demo_attrs[] = {
	&dev_attr_access.attr,
	&dev_attr_access_count.attr,
	&dev_attr_simulate_wakeup.attr,
	NULL,
};
ATTRIBUTE_GROUPS(pm_demo);

static int pm_demo_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct pm_demo_data *data;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	dev_set_drvdata(dev, data);

	/* Policy zuerst konfigurieren, danach das Gerät als aktiv melden */
	pm_runtime_set_autosuspend_delay(dev, PM_DEMO_AUTOSUSPEND_DELAY_MS);
	pm_runtime_use_autosuspend(dev);
	data->shadow_reg = PM_DEMO_REG_RESET_VALUE;
	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

	device_init_wakeup(dev, true);

	dev_info(dev, "probe: Runtime-PM aktiviert (autosuspend=%dms)\n",
		 PM_DEMO_AUTOSUSPEND_DELAY_MS);
	return 0;
}

static int pm_demo_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	device_init_wakeup(dev, false);
	pm_runtime_disable(dev);
	dev_info(dev, "remove: Runtime-PM deaktiviert\n");
	return 0;
}

static const struct platform_device_id pm_demo_id_table[] = {
	{ "pm-demo", 0 },
	{ }
};
MODULE_DEVICE_TABLE(platform, pm_demo_id_table);

static struct platform_driver pm_demo_driver = {
	.probe = pm_demo_probe,
	.remove = pm_demo_remove,
	.id_table = pm_demo_id_table,
	.driver = {
		.name = "pm-demo",
		.pm = &pm_demo_pm_ops,
		.dev_groups = pm_demo_groups,
	},
};

module_platform_driver(pm_demo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Internals Training");
MODULE_DESCRIPTION("Platform-Treiber mit Runtime-PM/Wakeup für Übung 5.06");
