#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/smp.h>

struct demo_ctx {
	struct timer_list timer;
	struct work_struct work;
	atomic_t runs;
	atomic_t coalesced;
	bool stopping;
};

static struct demo_ctx demo;

static unsigned int period_ms = 500;
module_param(period_ms, uint, 0644);
MODULE_PARM_DESC(period_ms, "Timer period in ms");

static unsigned int work_ms = 100;
module_param(work_ms, uint, 0644);
MODULE_PARM_DESC(work_ms, "Simulated work in ms");

static void demo_work_fn(struct work_struct *work)
{
	struct demo_ctx *ctx = container_of(work, struct demo_ctx, work);
	int n = atomic_inc_return(&ctx->runs);

	msleep(READ_ONCE(work_ms));
	pr_info("wqt_demo: run=%d cpu=%d\n", n, raw_smp_processor_id());
}

static void demo_timer_fn(struct timer_list *timer)
{
	struct demo_ctx *ctx = from_timer(ctx, timer, timer);
	unsigned int p;

	if (READ_ONCE(ctx->stopping))
		return;

	if (!schedule_work(&ctx->work))
		atomic_inc(&ctx->coalesced);

	p = max_t(unsigned int, READ_ONCE(period_ms), 1);
	mod_timer(&ctx->timer, jiffies + msecs_to_jiffies(p));
}

static int __init wqt_demo_init(void)
{
	if (!period_ms)
		return -EINVAL;

	atomic_set(&demo.runs, 0);
	atomic_set(&demo.coalesced, 0);
	WRITE_ONCE(demo.stopping, false);

	INIT_WORK(&demo.work, demo_work_fn);
	timer_setup(&demo.timer, demo_timer_fn, 0);
	mod_timer(&demo.timer, jiffies + msecs_to_jiffies(period_ms));

	return 0;
}

static void __exit wqt_demo_exit(void)
{
	WRITE_ONCE(demo.stopping, true);
	timer_shutdown_sync(&demo.timer);
	cancel_work_sync(&demo.work);

	pr_info("wqt_demo: runs=%d coalesced=%d\n",
		atomic_read(&demo.runs), atomic_read(&demo.coalesced));
}

module_init(wqt_demo_init);
module_exit(wqt_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Training");
MODULE_DESCRIPTION("Periodischer Timer treibt eine Workqueue - Trainingsbeispiel");
