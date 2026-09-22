#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>

static unsigned int interval_ms = 1000;
static bool verbose = true;
static void heartbeat(struct work_struct *work);
static DECLARE_DELAYED_WORK(heartbeat_work, heartbeat);

module_param(interval_ms, uint, 0644);
MODULE_PARM_DESC(interval_ms, "Heartbeat interval in ms");

module_param(verbose, bool, 0644);
MODULE_PARM_DESC(verbose, "Enable heartbeat messages");

static void heartbeat(struct work_struct *work)
{
    unsigned int ms = READ_ONCE(interval_ms);
    if (READ_ONCE(verbose))
{
        pr_info("configurable: heartbeat %u ms\n", ms);
	}

    schedule_delayed_work(&heartbeat_work, msecs_to_jiffies(ms));
}

static int __init configurable_init(void)
{
    schedule_delayed_work(&heartbeat_work, 0);
    pr_info("configurable: loaded\n");
    return 0;
}

static void __exit configurable_exit(void)
{
    cancel_delayed_work_sync(&heartbeat_work);
    pr_info("configurable: removed\n");
}

module_init(configurable_init);
module_exit(configurable_exit);
MODULE_LICENSE("GPL");
