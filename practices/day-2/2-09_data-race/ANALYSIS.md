# Analysis: Ausfuehrungskontext und Locking

## Ausfuehrungskontext

Jeder Worker aus `test_data_race.sh` fuehrt einen Userspace-Schreibzugriff auf
`/dev/data-race` aus. Der Kernel bearbeitet die zugehoerigen Operationen im
Prozesskontext des jeweiligen Workers:

```text
Shell-Worker
  -> open("/dev/data-race")
  -> write("reserve")
       -> data_race_write()
          -> reserve_slot()
  -> close()
       -> data_race_release()
```

Mehrere Worker koennen gleichzeitig auf unterschiedlichen CPUs oder
verschachtelt auf derselben CPU laufen. `free_slots` ist dabei gemeinsame
Modulzustandsvariable. Der pro Dateideskriptor gespeicherte Zustand
`state->reserved` verhindert nur eine zweite Reservierung durch denselben
Dateideskriptor; er schuetzt `free_slots` nicht vor anderen Workern.

`reserve_slot()` verwendet optional `usleep_range()`. Der Code darf deshalb
schlafen und laeuft nicht in einem atomaren Kontext. Insbesondere handelt es
sich weder um einen Hard-IRQ- noch einen SoftIRQ-Handler.

## Geeigneter Lock

Ein `mutex` ist passend, weil er im Prozesskontext verwendet werden darf und
wartende Worker schlafen legt. Ein `spinlock_t` waere falsch: Unter einem
Spinlock darf `usleep_range()` nicht aufgerufen werden.

Der Mutex muss den gesamten Check-Then-Act-Abschnitt sowie das Freigeben
schuetzen:

```c
#include <linux/mutex.h>

static DEFINE_MUTEX(slot_lock);

static int reserve_slot(void)
{
	int ret = 0;

	mutex_lock(&slot_lock);

	if (free_slots == 0) {
		ret = -ENOSPC;
		goto out;
	}

	if (delay_us)
		usleep_range(delay_us, delay_us + 100);

	free_slots--;
out:
	mutex_unlock(&slot_lock);
	return ret;
}

static void release_slot(void)
{
	mutex_lock(&slot_lock);
	free_slots++;
	mutex_unlock(&slot_lock);
}
```

Damit kann nach dem Check kein anderer Worker `free_slots` veraendern. Ein
zweiter Worker wartet auf den Mutex und erhaelt nach dem ersten erfolgreichen
Write korrekt `-ENOSPC`.

## Erweiterung auf andere Kontexte

Falls dieselbe Zustandsvariable spaeter auch aus Hard-IRQ- oder
SoftIRQ-Kontext verwendet wird, ist ein Mutex dort nicht zulaessig. In diesem
Fall muss der schlafende Teil in Prozesskontext verschoben oder der
nicht-schlafende kritische Abschnitt mit einem passenden Spinlock geschuetzt
werden.