// SPDX-License-Identifier: GPL-2.0

#include <linux/init.h>
#include <linux/module.h>

extern int ext_add(int a, int b);
extern int ext_gpl_add(int a, int b);

static int __init consumer_init(void)
{
	int normal_result;
	int gpl_result;

	normal_result = ext_add(10, 20);
	gpl_result = ext_gpl_add(30, 40);

	pr_info("consumer: ext_add(10, 20) = %d\n", normal_result);
	pr_info("consumer: ext_gpl_add(30, 40) = %d\n", gpl_result);

	return 0;
}

static void __exit consumer_exit(void)
{
	pr_info("consumer: module unloaded\n");
}

module_init(consumer_init);
module_exit(consumer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kernel Training");
MODULE_DESCRIPTION("Consumer of external kernel symbols");
