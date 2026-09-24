# Exkurs: Externe Symbole in Linux-Kernelmodulen

## Ziel der Übung

In dieser Übung stellt ein Kernelmodul Funktionen bereit, die von einem zweiten Kernelmodul verwendet werden. Dabei werden normale Exporte mit `EXPORT_SYMBOL()` und GPL-geschützte Exporte mit `EXPORT_SYMBOL_GPL()` untersucht. Typische Fehler werden bewusst provoziert und anschließend analysiert.

**Dauer:** ca. 75–90 Minuten  
**Voraussetzungen:** Grundlagen zu Kernelmodulen, Kbuild, `insmod`, `rmmod` und `dmesg`

## Lernziele

Die Teilnehmenden können:

- Funktionen eines Kernelmoduls exportieren und in einem anderen Modul aufrufen.
- die Aufgaben von `extern`, `EXPORT_SYMBOL()` und `EXPORT_SYMBOL_GPL()` unterscheiden.
- Symbolinformationen mit `Module.symvers`, `nm`, `modinfo` und `/proc/kallsyms` untersuchen.
- Abhängigkeiten zwischen zwei Out-of-Tree-Modulen korrekt auflösen.
- Build- und Ladefehler bei fehlenden oder GPL-geschützten Symbolen diagnostizieren.

## Versuchsaufbau

Es werden zwei Module erstellt:

| Modul | Aufgabe |
|---|---|
| `provider.ko` | Definiert und exportiert zwei Funktionen |
| `consumer.ko` | Importiert und verwendet die Funktionen |

Der Provider stellt `ext_add()` als normales Symbol und `ext_gpl_add()` als GPL-only-Symbol bereit.

```mermaid
flowchart LR
    C["consumer.ko"] -->|"ext_add()"| P["provider.ko"]
    C -->|"ext_gpl_add()"| P
    P --> S["Kernel-Symboltabelle"]
```

## 1. Arbeitsverzeichnis vorbereiten

```bash
mkdir external-symbols
cd external-symbols
```

Unter Debian oder Ubuntu werden ein Compiler, `make` und die zum laufenden Kernel passenden Header benötigt:

```bash
sudo apt install build-essential linux-headers-$(uname -r)
```

Build-Verzeichnis prüfen:

```bash
test -d /lib/modules/$(uname -r)/build && \
    echo "Kernel-Build-Verzeichnis vorhanden"
```

## 2. Provider-Modul erstellen

Datei `provider.c`:

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

### Verständnisfragen

1. Warum reicht eine nicht mit `static` deklarierte Funktion für die Nutzung durch andere Module nicht aus?
2. Welche Information erzeugt `EXPORT_SYMBOL()`?
3. Welche zusätzliche Einschränkung bewirkt `EXPORT_SYMBOL_GPL()`?

## 3. Consumer-Modul erstellen

Datei `consumer.c`:

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

    pr_info("consumer: ext_add(10, 20) = %d\n",
            normal_result);
    pr_info("consumer: ext_gpl_add(30, 40) = %d\n",
            gpl_result);

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

`extern` teilt dem Compiler lediglich mit, dass eine Funktion an anderer Stelle definiert ist. Es exportiert kein Symbol und garantiert nicht, dass der Kernel das Symbol beim Laden auflösen kann. In realen Projekten sollten die Deklarationen in einem gemeinsamen Header stehen.

## 4. Gemeinsames Kbuild-Makefile

Datei `Makefile`:

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

Module bauen:

```bash
make
```

Durch den gemeinsamen Build kennt `modpost` die vom Provider exportierten und die vom Consumer importierten Symbole. Dies ist die bevorzugte Variante für voneinander abhängige externe Module.

## 5. Symbolinformationen untersuchen

Exportinformationen anzeigen:

```bash
grep -E 'ext_add|ext_gpl_add' Module.symvers
```

Die Ausgabe enthält unter anderem Symbolname und Exporttyp:

```text
<CRC>  ext_add      provider  EXPORT_SYMBOL
<CRC>  ext_gpl_add  provider  EXPORT_SYMBOL_GPL
```

Bei aktiviertem `CONFIG_MODVERSIONS` enthält `Module.symvers` außerdem eine CRC über das Symbolinterface. Sie dient beim Laden als ABI-Kompatibilitätsprüfung.

Definierte Symbole im Provider anzeigen:

```bash
nm provider.ko | grep ext_
```

Nicht aufgelöste Referenzen im Consumer anzeigen:

```bash
nm -u consumer.ko | grep ext_
```

Erwartete Kennzeichnung:

```text
U ext_add
U ext_gpl_add
```

`U` bedeutet *undefined*: Das Symbol ist nicht im Consumer definiert und muss beim Laden durch den Kernel beziehungsweise ein anderes geladenes Modul aufgelöst werden.

## 6. Fehlerfall: Provider fehlt beim Laden

Zunächst wird absichtlich nur der Consumer geladen:

```bash
sudo insmod consumer.ko
```

Mögliche Ausgabe:

```text
insmod: ERROR: could not insert module consumer.ko: Unknown symbol in module
```

Kernelmeldungen auswerten:

```bash
sudo dmesg | tail -n 20
```

Typische Meldungen sind:

```text
consumer: Unknown symbol ext_add
consumer: Unknown symbol ext_gpl_add
```

Der Build war erfolgreich, weil `modpost` die Symbole aus dem gemeinsamen Build kannte. Beim Laden sind sie jedoch noch nicht verfügbar, da `provider.ko` nicht geladen ist.

## 7. Module in korrekter Reihenfolge laden

```bash
sudo insmod provider.ko
sudo insmod consumer.ko
sudo dmesg | tail -n 20
```

Erwartete Meldungen:

```text
provider: module loaded
consumer: ext_add(10, 20) = 30
consumer: ext_gpl_add(30, 40) = 70
```

Modulabhängigkeit untersuchen:

```bash
modinfo consumer.ko
lsmod | grep -E 'provider|consumer'
```

Geladene Symbole suchen:

```bash
sudo grep -E 'ext_add|ext_gpl_add' /proc/kallsyms
```

## 8. Referenzzählung beobachten

Solange der Consumer geladen ist, kann der Provider nicht entfernt werden:

```bash
sudo rmmod provider
```

Erwartung:

```text
rmmod: ERROR: Module provider is in use by: consumer
```

Korrekte Reihenfolge:

```bash
sudo rmmod consumer
sudo rmmod provider
```

Die Symbolabhängigkeit erhöht die Referenz auf das bereitstellende Modul. Dadurch verhindert der Kernel, dass ausführbarer Code entfernt wird, auf den noch ein anderes Modul verweist.

## 9. GPL-Fehler provozieren

Im Consumer wird die Lizenzangabe geändert:

```c
MODULE_LICENSE("Proprietary");
```

Anschließend neu bauen:

```bash
make clean
make
```

Typischer Fehler während `MODPOST`:

```text
ERROR: modpost: GPL-incompatible module consumer.ko
uses GPL-only symbol 'ext_gpl_add'
```

### Fehleranalyse

- Der C-Compiler kann die Quelldatei zunächst übersetzen, da eine gültige Funktionsdeklaration existiert.
- `modpost` gleicht die importierten Symbole mit den Exportinformationen ab.
- `ext_gpl_add` ist als `EXPORT_SYMBOL_GPL` gekennzeichnet.
- Der Consumer deklariert mit `MODULE_LICENSE("Proprietary")` keine GPL-kompatible Lizenz.
- Deshalb wird die Modulerzeugung abgebrochen.

Das normale Symbol `ext_add`, das mit `EXPORT_SYMBOL()` exportiert wurde, ist von dieser technischen GPL-Prüfung nicht betroffen.

> **Wichtig:** Das bloße Eintragen von `MODULE_LICENSE("GPL")` ändert nicht automatisch die tatsächliche rechtliche Lizenz eines Moduls. Die Metadaten müssen mit der realen Lizenzierung des Quellcodes übereinstimmen.

## 10. GPL-Fehler beheben

### Möglichkeit A: GPL-kompatibles Modul

Wenn Quellcode und Lizenzierung dies tatsächlich erlauben:

```c
MODULE_LICENSE("GPL");
```

Danach neu bauen:

```bash
make clean
make
```

### Möglichkeit B: GPL-only-Symbol nicht verwenden

Aus `consumer.c` werden Deklaration und Aufruf von `ext_gpl_add()` entfernt. Der Consumer verwendet anschließend ausschließlich `ext_add()`.

### Möglichkeit C: Exportart des eigenen Providers ändern

Beim selbst entwickelten Provider könnte der Eigentümer bewusst einen normalen Export anbieten:

```c
EXPORT_SYMBOL(ext_gpl_add);
```

Bei bestehenden Symbolen des Linux-Kernels oder fremden Modulen kann der Consumer die vorgegebene Exportart nicht ändern.

## 11. Getrennte Builds mit `KBUILD_EXTRA_SYMBOLS`

Werden Provider und Consumer in getrennten Verzeichnissen gebaut, kennt `modpost` beim Consumer-Build zunächst nicht die Symbole des Providers:

```text
external-symbols/
├── provider/
│   ├── provider.c
│   ├── Makefile
│   └── Module.symvers
└── consumer/
    ├── consumer.c
    └── Makefile
```

Provider zuerst bauen:

```bash
make -C provider
```

Consumer mit der Symboltabelle des Providers bauen:

```bash
make -C consumer \
    KBUILD_EXTRA_SYMBOLS="$(realpath provider/Module.symvers)"
```

Ohne diese Information entsteht typischerweise:

```text
ERROR: modpost: "ext_add" [consumer.ko] undefined!
```

`KBUILD_EXTRA_SYMBOLS` stellt `modpost` die Exportinformationen separat gebauter externer Module zur Verfügung. Es lädt das Provider-Modul jedoch nicht automatisch in den laufenden Kernel.

## 12. Typische Fehlerbilder

| Fehlermeldung | Phase | Wahrscheinliche Ursache |
|---|---|---|
| `implicit declaration of function` | Compiler | Funktionsdeklaration oder Header fehlt |
| `"ext_add" [...] undefined!` | `modpost` | Symbol nicht exportiert oder `Module.symvers` unbekannt |
| `GPL-incompatible module ... uses GPL-only symbol` | `modpost` | Nicht GPL-kompatibles Modul verwendet ein GPL-only-Symbol |
| `Unknown symbol in module` | Modul-Loader | Provider nicht geladen oder Symbol im laufenden Kernel nicht verfügbar |
| `disagrees about version of symbol` | Modul-Loader | Abweichende Symbol-CRC oder inkompatibler Build |
| `Module provider is in use` | `rmmod` | Ein geladenes Modul verwendet Symbole des Providers |

## 13. Abschlussaufgabe

Erweitern Sie den Provider um folgende Funktion:

```c
int ext_multiply(int a, int b);
```

### Anforderungen

1. Funktion mit `EXPORT_SYMBOL_GPL()` exportieren.
2. Funktion im Consumer deklarieren und aufrufen.
3. Exporttyp in `Module.symvers` nachweisen.
4. Consumer als GPL-kompatibles Modul erfolgreich bauen und laden.
5. Anschließend mit `MODULE_LICENSE("Proprietary")` den GPL-Fehler provozieren.
6. Fehlerphase, Ursache und zwei mögliche Lösungen dokumentieren.

## Auswertung

Die Symbolnutzung durchläuft mehrere Ebenen:

| Ebene | Aufgabe |
|---|---|
| `extern`/Header | Deklaration für den C-Compiler |
| `EXPORT_SYMBOL*()` | Veröffentlichung in der Kernel-Symboltabelle |
| `Module.symvers` | Informationen für Kbuild und `modpost` |
| `modpost` | Prüfung von Existenz, Version und GPL-Kompatibilität |
| Modul-Loader | Auflösung gegen aktuell geladene Kernel- und Modulsymbole |

**Merksatz:** `extern` deklariert ein Symbol, `EXPORT_SYMBOL*()` veröffentlicht es, `Module.symvers` beschreibt es für den Build und der Modul-Loader löst es zur Laufzeit auf.

## Weiterführende Referenzen

- Linux Kernel Documentation: *Building External Modules*  
  <https://docs.kernel.org/kbuild/modules.html>
- Linux Kernel Documentation: *Linux kernel licensing rules*  
  <https://docs.kernel.org/process/license-rules.html>
- Linux Kernel Documentation: *Symbol Namespaces*  
  <https://docs.kernel.org/core-api/symbol-namespaces.html>
