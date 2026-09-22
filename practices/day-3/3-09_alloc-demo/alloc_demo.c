// SPDX-License-Identifier: GPL-2.0
/*
 * alloc_demo.c - Trainingsbeispiel: dynamische Speicherallokation mit
 * vollständigen Fehler- und Freigabepfaden.
 *
 * Drei Allokationsebenen (Pointer-Array -> Objekte -> Payload) und ein
 * Laborparameter (fail_after) erzwingen einen reproduzierbaren, partiellen
 * Rückrollpfad: jeder Ladeversuch endet entweder vollständig initialisiert
 * oder vollständig aufgeräumt.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/errno.h>

struct demo_item {
	u32 id;
	size_t len;
	u8 *payload;
};

static struct demo_item **items;
static unsigned int item_count;

static uint count = 4;
module_param(count, uint, 0444);
MODULE_PARM_DESC(count, "Anzahl der zu erzeugenden Objekte");

static uint payload_size = 128;
module_param(payload_size, uint, 0444);
MODULE_PARM_DESC(payload_size, "Groesse des Payloads je Objekt in Bytes");

static int fail_after = -1;
module_param(fail_after, int, 0444);
MODULE_PARM_DESC(fail_after,
	"Simuliert einen Allokationsfehler ab diesem Index (-1 = aus)");

static int validate_params(void)
{
	if (!count || count > 1024)
		return -EINVAL;

	if (!payload_size || payload_size > PAGE_SIZE)
		return -EINVAL;

	return 0;
}

static struct demo_item *item_create(u32 id, size_t len)
{
	struct demo_item *item;

	item = kzalloc(sizeof(*item), GFP_KERNEL);
	if (!item)
		return NULL;

	item->payload = kmalloc(len, GFP_KERNEL);
	if (!item->payload)
		goto out_free_item;

	item->id = id;
	item->len = len;
	return item;

out_free_item:
	kfree(item);
	return NULL;
}

static void item_destroy(struct demo_item *item)
{
	if (!item)
		return;

	kfree(item->payload);
	kfree(item);
}

static int create_items(void)
{
	unsigned int i;

	items = kcalloc(count, sizeof(*items), GFP_KERNEL);
	if (!items)
		return -ENOMEM;

	for (i = 0; i < count; i++) {
		if (fail_after >= 0 && i == fail_after)
			goto out_free_items;

		items[i] = item_create(i, payload_size);
		if (!items[i])
			goto out_free_items;
	}

	item_count = count;
	pr_info("alloc_demo: %u Objekte a %u Bytes angelegt\n", count, payload_size);
	return 0;

out_free_items:
	while (i > 0) {
		i--;
		item_destroy(items[i]);
	}
	kfree(items);
	items = NULL;
	return -ENOMEM;
}

static int __init demo_init(void)
{
	int ret = validate_params();

	if (ret)
		return ret;

	return create_items();
}

static void __exit demo_exit(void)
{
	while (item_count > 0) {
		item_count--;
		item_destroy(items[item_count]);
	}
	kfree(items);
	pr_info("alloc_demo: entladen, alle Ressourcen freigegeben\n");
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Training");
MODULE_DESCRIPTION("Dynamische Speicherallokation mit vollstaendigen Fehler- und Freigabepfaden");
