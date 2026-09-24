# Übung: Externe Symbole in Linux-Kernelmodulen

Diese Übung baut auf dem Exkurs zu externen Symbolen in Linux-Kernelmodulen auf und zeigt, wie ein Modul Funktionen bereitstellt und ein zweites Modul diese importiert. Dabei werden die Exportmechanismen `EXPORT_SYMBOL()` und `EXPORT_SYMBOL_GPL()` praktisch untersucht.

## Lernziele

- Funktionen eines Kernelmoduls exportieren und in einem anderen Modul nutzen
- `extern`, `EXPORT_SYMBOL()` und `EXPORT_SYMBOL_GPL()` voneinander unterscheiden
- Symbolinformationen mit `Module.symvers`, `nm`, `modinfo` und `/proc/kallsyms` lesen
- Abhängigkeiten zwischen Modulen und Ladefehler nachvollziehen
- GPL-Fehler im Build und beim Laden erkennen und beheben

## Vorbereitung

- Linux-System oder VM mit passenden Kernel-Headern
- `build-essential` und `linux-headers-$(uname -r)` installiert
- Root-Rechte für `insmod`, `rmmod`, `dmesg` und `modinfo`

> Hinweis zur Umgebung: Der Build und die Symbolprüfung funktionieren auch ohne Root. Der eigentliche Laufzeit-Test mit `insmod` benötigt jedoch `sudo` bzw. passende Admin-Rechte. In abgesicherten Umgebungen kann dieser Schritt deshalb nicht ausgeführt werden, obwohl die Module selbst korrekt bauen.

## Aufbau

Es werden zwei Module erstellt:

| Modul | Aufgabe |
|---|---|
| `provider.ko` | stellt Funktionen bereit |
| `consumer.ko` | nutzt die bereitgestellten Funktionen |

Der Provider exportiert:

- `ext_add()` mit `EXPORT_SYMBOL()`
- `ext_gpl_add()` mit `EXPORT_SYMBOL_GPL()`

## Verzeichnisstruktur

```text
externe_symbole/
├── Makefile
├── README.md
├── provider.c
├── consumer.c
└── Module.symvers (nach dem Build)
```

## 1. Provider-Modul erstellen

Das erste Modul definiert zwei Funktionen und exportiert sie.

```c
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
```

### Aufgaben

1. Warum reicht eine einfache Funktion nicht aus, wenn ein zweites Modul sie nutzen soll?
2. Welche Folge hat `EXPORT_SYMBOL()`?
3. Welche Zusatzregel gilt bei `EXPORT_SYMBOL_GPL()`?

### Erwartete Beobachtung

Die Funktion wird im Kernel-Symbolraum veröffentlicht. Ein zweites Modul kann sie dann mit `extern` deklarieren und nutzen, sofern der Kernel sie zur Laufzeit auflösen kann.

---

## 2. Consumer-Modul erstellen

Das zweite Modul verwendet die exportierten Funktionen.

```c
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
```

### Aufgaben

1. Warum ist `extern` notwendig?
2. Was bedeutet es, wenn `nm -u consumer.ko` auf `U ext_add` zeigt?
3. Welche Rolle spielt das gemeinsame Kbuild für `modpost`?

### Erwartete Beobachtung

`extern` ist nur eine Deklaration. Der Compiler weiß dann, dass die Funktion irgendwo existiert; der Modul-Loader muss sie später im laufenden System auflösen.

---

## 3. Gemeinsames Makefile

```make
obj-m += provider.o
obj-m += consumer.o

KDIR ?= /lib/modules/$(shell uname -r)/build

.PHONY: all clean

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
```

### Aufgaben

1. Bauen Sie die Module mit `make`.
2. Prüfen Sie nach dem Build, was in `Module.symvers` steht.
3. Analysieren Sie, welche Symbole im Provider und im Consumer sichtbar sind.

### Erwartete Beobachtung

Nach dem Build enthält `Module.symvers` die Export-Informationen. `nm provider.ko` zeigt die definierten Symbole. `nm -u consumer.ko` zeigt die Import-Referenzen.

---

## 4. Fehlerfall: Provider fehlt

Laden Sie zunächst nur den Consumer:

```bash
sudo insmod consumer.ko
```

### Aufgaben

1. Was passiert beim Laden?
2. Welches Symbol fehlt?
3. Warum kann `modpost` den Build erfolgreich abschließen, obwohl ein Modul beim Laden noch fehlt?

### Erwartete Beobachtung

Der Loader meldet einen unbekannten Symbolfehler, weil der Provider noch nicht geladen wurde.

---

## 5. Korrekte Reihenfolge beim Laden

```bash
sudo insmod provider.ko
sudo insmod consumer.ko
sudo dmesg | tail -n 20
```

### Aufgaben

1. Welches Ergebnis liefert `ext_add(10, 20)`?
2. Welches Ergebnis liefert `ext_gpl_add(30, 40)`?
3. Welche Symbole erscheinen in `/proc/kallsyms`?

### Erwartete Beobachtung

- `ext_add(10, 20) = 30`
- `ext_gpl_add(30, 40) = 70`

---

## 6. Referenzzählung und `rmmod`

```bash
sudo rmmod provider
```

### Aufgaben

1. Warum lässt der Kernel das Entfernen des Providers nicht zu, solange der Consumer noch geladen ist?
2. Welche korrekte Reihenfolge ist für das Entfernen?

### Erwartete Beobachtung

Der Provider ist nach einem Modul-Import noch in Verwendung. `rmmod` verweigert den Vorgang, bis der Consumer entfernt wurde.

---

## 7. GPL-Fehler provozieren

Ändern Sie im Consumer die Lizenzangabe:

```c
MODULE_LICENSE("Proprietary");
```

Danach:

```bash
make clean
make
```

### Aufgaben

1. Welche Fehlermeldung erscheint?
2. Warum ist `ext_gpl_add()` nicht nutzbar, wenn der Consumer nicht GPL-kompatibel ist?
3. Welche Auswirkung hat das auf den Build?

### Erwartete Beobachtung

`modpost` meldet einen GPL-Konflikt, weil `ext_gpl_add()` als `EXPORT_SYMBOL_GPL()` gekennzeichnet ist und der Consumer eine nicht-GPL-Lizenz angibt.

---

## 8. Fehler beheben

### Möglichkeit A: GPL-Lizenz verwenden

```c
MODULE_LICENSE("GPL");
```

### Möglichkeit B: GPL-only-Symbol nicht verwenden

Entferne `ext_gpl_add()` aus der Nutzung und aus den Deklarationen.

### Aufgaben

1. Nenne zwei mögliche Lösungswege.
2. Warum ist das Erwartete Verhalten im Consumer nach der Korrektur wieder korrekt?

---

## 9. Getrennter Build mit `KBUILD_EXTRA_SYMBOLS`

Wenn Provider und Consumer in getrennten Verzeichnissen gebaut werden, muss `modpost` die Symboltabelle des Providers kennen.

```bash
make -C provider
make -C consumer KBUILD_EXTRA_SYMBOLS="$(realpath provider/Module.symvers)"
```

### Aufgaben

1. Warum ist `KBUILD_EXTRA_SYMBOLS` notwendig?
2. Was passiert ohne diese Angabe?
3. Warum lädt der Build das Provider-Modul nicht automatisch?

### Erwartete Beobachtung

Ohne zusätzliche Symboltabelle kennt `modpost` die Exporte des Providers nicht und die Verknüpfung ist fehlerhaft.

---

## Musterlösung

### 1. Warum reicht eine normale Funktion nicht aus?

Eine Funktion ist für den C-Compiler nur dann nutzbar, wenn sie deklariert ist. Für andere Module im laufenden Kernel muss sie zusätzlich in der Symboltabelle veröffentlicht werden.

### 2. Wie unterscheiden sich `EXPORT_SYMBOL()` und `EXPORT_SYMBOL_GPL()`?

`EXPORT_SYMBOL()` macht ein Symbol für alle Modul-Lizenzen nutzbar. `EXPORT_SYMBOL_GPL()` erlaubt nur GPL-kompatible Module den Zugriff.

### 3. Warum sind `Module.symvers` wichtig?

Sie enthalten die Exportinformationen, die `modpost` für die Prüfung und Verknüpfung verwendet. Sie ermöglichen die Bewertung von Versions- und Lizenzkonflikten.

### 4. Warum schlägt ein Build fehl, wenn der Consumer `Proprietary` meldet?

Weil `ext_gpl_add()` nur für GPL-kompatible Module freigegeben wurde. Der Kernel prüft die Lizenzkonformität beim Build.

### 5. Warum ist ein ungeladenes Provider-Modul ein Problem?

Der Loader kann das Symbol nicht auflösen, da es im laufenden Kernel nicht verfügbar ist. Das Modul bleibt auf Grund dessen unverfügbar.

---

## Abschlussaufgabe

Erweitern Sie den Provider um die Funktion `ext_multiply()` und exportieren Sie sie mit `EXPORT_SYMBOL_GPL()`. Danach:

1. deklarieren und nutzen Sie das Symbol im Consumer,
2. prüfen Sie die Exportdaten in `Module.symvers`,
3. bauen Sie das Modul erfolgreich,
4. provozieren Sie wieder den GPL-Fehler mit `MODULE_LICENSE("Proprietary")`,
5. dokumentieren Sie Ursache und Lösung.

## Merksatz

> `extern` deklariert ein Symbol, `EXPORT_SYMBOL*()` veröffentlicht es, `Module.symvers` beschreibt es für den Build, und der Loader löst es zur Laufzeit auf.

## Weiterführende Links

- Linux Kernel Documentation: Building External Modules
- Linux Kernel Documentation: License Rules
- Linux Kernel Documentation: Symbol Namespaces
