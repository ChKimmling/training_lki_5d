// SPDX-License-Identifier: GPL-2.0
/*
 * labchar.c - Trainingsbeispiel: einfacher Character-Device-Treiber mit
 * gepuffertem read/write, symmetrischem Erwerb/Rückbau und einem
 * konsistenten Kernel-Snapshot fürs Locking.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/err.h>
#include <linux/minmax.h>

#define LAB_NAME "labchar"
#define LAB_SIZE 128

struct lab_dev {
	struct cdev cdev;
	struct mutex lock;
	char data[LAB_SIZE];
	size_t len;
};

static dev_t lab_devt;
static struct class *lab_class;
static struct lab_dev lab;

static int lab_open(struct inode *inode, struct file *file)
{
	struct lab_dev *d;

	d = container_of(inode->i_cdev, struct lab_dev, cdev);
	file->private_data = d;
	return 0;
}

static int lab_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t lab_read(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	struct lab_dev *d = file->private_data;
	char tmp[LAB_SIZE];
	size_t len;

	mutex_lock(&d->lock);
	len = d->len;
	memcpy(tmp, d->data, len);
	mutex_unlock(&d->lock);

	return simple_read_from_buffer(buf, count, ppos, tmp, len);
}

static ssize_t lab_write(struct file *file, const char __user *buf,
			  size_t count, loff_t *ppos)
{
	struct lab_dev *d = file->private_data;
	char tmp[LAB_SIZE];
	size_t n = min(count, sizeof(tmp) - 1);

	if (copy_from_user(tmp, buf, n))
		return -EFAULT;
	tmp[n] = '\0';

	mutex_lock(&d->lock);
	memcpy(d->data, tmp, n + 1);
	d->len = n;
	mutex_unlock(&d->lock);

	return n;
}

static const struct file_operations lab_fops = {
	.owner   = THIS_MODULE,
	.open    = lab_open,
	.release = lab_release,
	.read    = lab_read,
	.write   = lab_write,
	.llseek  = no_llseek,
};

static int __init lab_init(void)
{
	struct device *device;
	int ret;

	mutex_init(&lab.lock);

	ret = alloc_chrdev_region(&lab_devt, 0, 1, LAB_NAME);
	if (ret)
		return ret;

	cdev_init(&lab.cdev, &lab_fops);
	ret = cdev_add(&lab.cdev, lab_devt, 1);
	if (ret)
		goto err_region;

	lab_class = class_create(LAB_NAME);
	if (IS_ERR(lab_class)) {
		ret = PTR_ERR(lab_class);
		goto err_cdev;
	}

	device = device_create(lab_class, NULL, lab_devt, &lab, "labchar0");
	if (IS_ERR(device)) {
		ret = PTR_ERR(device);
		goto err_class;
	}

	return 0;

err_class:
	class_destroy(lab_class);
err_cdev:
	cdev_del(&lab.cdev);
err_region:
	unregister_chrdev_region(lab_devt, 1);
	return ret;
}

static void __exit lab_exit(void)
{
	device_destroy(lab_class, lab_devt);
	class_destroy(lab_class);
	cdev_del(&lab.cdev);
	unregister_chrdev_region(lab_devt, 1);
}

module_init(lab_init);
module_exit(lab_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Training");
MODULE_DESCRIPTION("Einfacher Character-Device-Treiber - Trainingsbeispiel");
