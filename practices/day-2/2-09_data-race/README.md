# data_race - Beispielmodul für einen Data Race

Minimales Kernel-Modul für ein Training zum Thema Data Races. Legt
`/dev/data-race` an und implementiert absichtlich unsynchronisiert den
klassischen check-then-act Fehlerpfad:

```c
static unsigned int free_slots = 1;

int reserve_slot(void)
{
	if (free_slots == 0)
		return -ENOSPC;

	free_slots--;

	return 0;
}
```

Jeder Zugriff (`open()`/`write()`) auf `/dev/data-race` ruft `reserve_slot()`
auf, `close()` gibt den Slot über `release_slot()` wieder frei. Da Check und
Dekrement nicht atomar sind, können zwei parallele Zugriffe beide den Check
passieren, bevor einer dekrementiert hat - `free_slots` wird dann mehrfach
reserviert bzw. läuft als `unsigned int` unter 0.

Mit dem Modulparameter `delay_us` lässt sich das Rennfenster künstlich
vergrößern (`insmod data_race.ko delay_us=100000`), um den Race im Training
zuverlässig zu reproduzieren.

## Bauen

```bash
make
```

## Testen

```bash
sudo ./test_data_race.sh [ITERATIONEN] [DELAY_US]
```

Das Skript lädt das Modul, lässt in jeder Iteration zwei Worker parallel
`echo reserve > /dev/data-race` ausführen und gibt am Ende die letzten 50
Zeilen aus `dmesg` aus. Ein Race zeigt sich, wenn mehrfach hintereinander
`open() ok` geloggt wird, obwohl `free_slots` bereits 0 war.

## Aufräumen

Das Skript entfernt das Modul beim Beenden automatisch (`trap ... EXIT`).
Manuell: `sudo rmmod data_race`.
