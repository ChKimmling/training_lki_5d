// SPDX-License-Identifier: GPL-2.0

#include <linux/init.h>
#include <linux/module.h>
#include <linux/export.h>

int ext_add(int a, int b)
{
	return a + b;
}
EXPORT_SYMBOL(ext_add);

int ext_gpl_add(int a, int b)
{
	return a + b;
}
EXPORT_SYMBOL_GPL(ext_gpl_add);

static int __init provider_init(void)
{
	pr_info("provider: module loaded\n");
	return 0;
}

static void __exit provider_exit(void)
{
	pr_info("provider: module unloaded\n");
}

module_init(provider_init);
module_exit(provider_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kernel Training");
MODULE_DESCRIPTION("Provider for exported kernel symbols");
