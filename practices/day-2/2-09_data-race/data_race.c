/**
 * @file data_race.c
 * @brief Trainingsmodul zur Demonstration eines Check-Then-Act Data Race.
 *
 * Das Modul stellt /dev/data-race bereit und implementiert absichtlich einen
 * unsynchronisierten Zugriff auf die Anzahl freier Slots.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>

#define DEVICE_NAME "data-race"

static unsigned int free_slots = 1;

static unsigned int delay_us;
module_param(delay_us, uint, 0644);
MODULE_PARM_DESC(delay_us,
	"Kuenstliche Verzoegerung (in Mikrosekunden) zwischen Check und "
	"Dekrement in reserve_slot(), um das Rennfenster fuer Trainingszwecke "
	"zu vergroessern (0 = aus).");

/**
 * @brief Reserviert einen freien Slot.
 *
 * Absichtlich unsynchronisiert: der Fehlerpfad aus der Trainingsvorlage.
 *
 * @return 0 bei erfolgreicher Reservierung, andernfalls -ENOSPC.
 */
int reserve_slot(void)
{
	if (free_slots == 0)
		return -ENOSPC;

	if (delay_us)
		usleep_range(delay_us, delay_us + 100);

	free_slots--;

	return 0;
}

/**
 * @brief Gibt einen zuvor reservierten Slot frei.
 */
static void release_slot(void)
{
	free_slots++;
}

/**
 * @brief Behandelt das Oeffnen des Data-Race-Geraets.
 *
 * @param inode Inode des Geraets.
 * @param file Geoeffnete Datei.
 * @return 0 bei Erfolg oder ein negativer Fehlercode.
 */
static int data_race_open(struct inode *inode, struct file *file)
{
	int ret = reserve_slot();

	if (ret)
		pr_info("data-race: open() abgelehnt, free_slots=%u (%d)\n",
			free_slots, ret);
	else
		pr_info("data-race: open() ok, free_slots=%u\n", free_slots);

	return ret;
}

/**
 * @brief Behandelt das Schliessen des Data-Race-Geraets.
 *
 * @param inode Inode des Geraets.
 * @param file Geoeffnete Datei.
 * @return Immer 0.
 */
static int data_race_release(struct inode *inode, struct file *file)
{
	release_slot();
	pr_info("data-race: release(), free_slots=%u\n", free_slots);
	return 0;
}

/**
 * @brief Akzeptiert beliebigen Schreibzugriff auf das Geraet.
 *
 * Beispiel: "echo reserve > /dev/data-race".
 *
 * @param file Geoeffnete Datei.
 * @param buf Puffer mit den Benutzerdaten.
 * @param count Anzahl der zu schreibenden Bytes.
 * @param ppos Aktuelle Position im Geraet.
 * @return Die Anzahl der akzeptierten Bytes.
 */
static ssize_t data_race_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	return count;
}

static const struct file_operations data_race_fops = {
	.owner   = THIS_MODULE,
	.open    = data_race_open,
	.write   = data_race_write,
	.release = data_race_release,
};

static struct miscdevice data_race_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEVICE_NAME,
	.fops  = &data_race_fops,
};

/**
 * @brief Registriert das Data-Race-Geraet.
 *
 * @return 0 bei erfolgreicher Registrierung oder ein negativer Fehlercode.
 */
static int __init data_race_init(void)
{
	int ret = misc_register(&data_race_dev);

	if (ret) {
		pr_err("data-race: misc_register fehlgeschlagen: %d\n", ret);
		return ret;
	}

	pr_info("data-race: geladen, /dev/%s angelegt, free_slots=%u\n",
		DEVICE_NAME, free_slots);
	return 0;
}

/**
 * @brief Deregistriert das Data-Race-Geraet.
 */
static void __exit data_race_exit(void)
{
	misc_deregister(&data_race_dev);
	pr_info("data-race: entfernt\n");
}

module_init(data_race_init);
module_exit(data_race_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Training");
MODULE_DESCRIPTION("Minimalbeispiel eines check-then-act Data Race fuer Trainingszwecke");
