# Exkurs: Symbole zwischen Linux-Kernelmodulen verwenden

## Grundidee

Ein Kernelmodul kann **Funktionen oder globale Variablen eines anderen Moduls** verwenden, wenn der Anbieter sie mit `EXPORT_SYMBOL()` oder `EXPORT_SYMBOL_GPL()` exportiert. Der Kernel löst die Referenzen beim Laden des verwendenden Moduls auf.

```text
provider.ko                            consumer.ko
  demo_add() ── exportiertes Symbol ──> demo_add(10, 20)
```

## 1. Funktion exportieren

**`provider.c`**:

```c
#include <linux/module.h>

int demo_add(int a, int b)
{
    return a + b;
}
EXPORT_SYMBOL_GPL(demo_add);

MODULE_LICENSE("GPL");
```

- `EXPORT_SYMBOL(name)`: normaler Export.
- `EXPORT_SYMBOL_GPL(name)`: Export nur für Module, deren deklarierte Lizenz der Kernel als GPL-kompatibel anerkennt.

Auch Variablen lassen sich exportieren; gemeinsam veränderliche Daten brauchen geeignete Synchronisation.

## 2. Funktion in einem zweiten Modul verwenden

Gemeinsame Headerdatei **`demo_api.h`**:

```c
#ifndef DEMO_API_H
#define DEMO_API_H
int demo_add(int a, int b);
#endif
```

**`consumer.c`**:

```c
#include <linux/module.h>
#include "demo_api.h"

static int __init consumer_init(void)
{
    pr_info("Ergebnis: %d\n", demo_add(10, 20));
    return 0;
}

static void __exit consumer_exit(void) { }

module_init(consumer_init);
module_exit(consumer_exit);
MODULE_LICENSE("GPL");
```

Die Headerdatei deklariert die Schnittstelle; sie exportiert selbst kein Symbol.

## 3. Beide Module bauen und laden

**Makefile** im gemeinsamen Verzeichnis:

```makefile
obj-m += provider.o
obj-m += consumer.o
```

```bash
make -C /lib/modules/$(uname -r)/build M="$PWD" modules
sudo insmod provider.ko
sudo insmod consumer.ko
sudo dmesg | tail
```

Erwartete Meldung: `Ergebnis: 30`. Beim Entladen zuerst den Consumer entfernen:

```bash
sudo rmmod consumer
sudo rmmod provider
```

## 4. Symbolauflösung und GPL-Prüfung

Beim Build prüft **`modpost`** Symbolreferenzen anhand der bekannten Exporte. Der **Kernel-Modul-Loader** löst sie beim Laden auf und prüft unter anderem die GPL-only-Beschränkung anhand von `MODULE_LICENSE()`.

```text
Quellcode → Kbuild / modpost → *.ko
                                |
                                v
                       Kernel-Modul-Loader
                                |
                 Symbol vorhanden und erlaubt?
                        /             \
                      Ja              Nein
                       |                |
                Referenz binden     Ladefehler
```

Bei getrennten Builds muss der Consumer-Build die Exporte des Providers kennen, z. B. über dessen `Module.symvers` mit `KBUILD_EXTRA_SYMBOLS`.

## 5. Analyse und Installation

| Befehl | Zweck |
|---|---|
| `grep demo_add Module.symvers` | Exporttyp prüfen |
| `nm -u consumer.ko` | Noch aufzulösende Symbole anzeigen |
| `modinfo consumer.ko` | Modulmetadaten und Abhängigkeiten anzeigen |
| `modprobe --show-depends consumer` | Ladeabhängigkeiten anzeigen |

Nach der Installation beider `.ko`-Dateien unter `/lib/modules/$(uname -r)/` und `sudo depmod -a` kann `modprobe consumer` die bekannten Abhängigkeiten automatisch laden.

**Merksatz:** `EXPORT_SYMBOL()` und `EXPORT_SYMBOL_GPL()` machen Kernel-Symbole für andere Module sichtbar; `modpost` prüft bekannte Exporte beim Build, und der Kernel löst die Referenzen einschließlich GPL-only-Prüfung beim Laden auf.
