# Exkurs: Linux-Kernel-Build mit Kconfig, Kbuild & Co.

## Überblick

Der Linux-Kernel besitzt ein eigenes Build-System, das mehrere Komponenten kombiniert:

```text
Kconfig   -> Was soll gebaut werden?
Kbuild    -> Wie werden Kernelteile gebaut?
Make      -> Steuert den gesamten Build
.config   -> Konkrete Kernel-Konfiguration
```

Der typische Ablauf ist:

```text
Kernel-Quellcode
      |
      v
Kconfig
      |
      v
.config
      |
      v
Kbuild / Make
      |
      v
Kernel + Module
```

## 1. Kconfig – Kernel konfigurieren

**Kconfig** beschreibt die verfügbaren Konfigurationsoptionen des Kernels.

Beispiele:

```text
CONFIG_NET
CONFIG_USB
CONFIG_EXT4_FS
CONFIG_DEBUG_KERNEL
```

Kconfig-Dateien befinden sich im gesamten Source Tree, zum Beispiel:

```text
Kconfig
drivers/Kconfig
fs/Kconfig
net/Kconfig
```

Ein Eintrag kann vereinfacht so aussehen:

```text
config MY_DRIVER
    tristate "My example driver"
    depends on PCI
```

Wichtige Typen:

```text
bool      -> y / n
tristate  -> y / m / n
string
int
hex
```

Bei `tristate` bedeutet:

```text
y = fest in den Kernel eingebaut
m = als Kernelmodul gebaut
n = nicht gebaut
```

## 2. `.config` – konkrete Build-Konfiguration

Die ausgewählten Optionen landen in:

```text
.config
```

Beispiel:

```text
CONFIG_NET=y
CONFIG_EXT4_FS=y
CONFIG_USB=m
# CONFIG_DEBUG_KERNEL is not set
```

Sie kann erzeugt oder verändert werden mit:

```bash
make defconfig
make menuconfig
make nconfig
make oldconfig
```

## 3. Kbuild – was wird kompiliert?

**Kbuild** beschreibt, welche Quellcodedateien zu welchen Kernelobjekten gehören.

Beispiel:

```make
obj-$(CONFIG_MY_DRIVER) += my_driver.o
```

Das bedeutet:

```text
CONFIG_MY_DRIVER=y -> fest einbauen
CONFIG_MY_DRIVER=m -> Modul bauen
CONFIG_MY_DRIVER=n -> nicht bauen
```

Mehrteilige Module:

```make
obj-$(CONFIG_MY_DRIVER) += my_driver.o
my_driver-y := core.o bus.o device.o
```

## 4. Zusammenspiel von Kconfig und Kbuild

```text
Kconfig
   |
   | entscheidet
   v
CONFIG_MY_DRIVER=m
   |
   v
Kbuild
   |
   | interpretiert
   v
my_driver.ko
```

Kurz:

> **Kconfig entscheidet, was aktiviert ist. Kbuild entscheidet, wie daraus Binärdateien entstehen.**

## 5. Top-Level-Makefile

Das Haupt-`Makefile` im Linux-Source-Tree koordiniert den gesamten Build.

Typischer Aufruf:

```bash
make -j$(nproc)
```

Dabei entstehen unter anderem:

```text
vmlinux
Kernel-Image
Kernelmodule
generierte Header
```

Architekturspezifische Images sind zum Beispiel:

```text
arch/x86/boot/bzImage
arch/arm64/boot/Image
```

## 6. Wichtige Build-Artefakte

```text
vmlinux        unkomprimiertes ELF-Kernel-Binary
System.map     Symboltabelle
Module.symvers exportierte Modulsymbole
modules.order  Reihenfolge gebauter Module
*.o            Objektdateien
*.ko           Kernelmodule
*.cmd          Build-Kommandos und Abhängigkeiten
```

## 7. In-Tree- und Out-of-Tree-Build

Direkt im Source Tree:

```bash
make
```

Sauberer ist häufig ein separater Output-Tree:

```bash
make O=../linux-build defconfig
make O=../linux-build menuconfig
make O=../linux-build -j$(nproc)
```

Dann gilt:

```text
linux/
   -> Source Tree

linux-build/
   -> .config
   -> generierte Dateien
   -> Build-Artefakte
```

Alternativ:

```bash
make KBUILD_OUTPUT=../linux-build
```

## 8. Architektur und Cross-Compilation

Die Zielarchitektur wird mit `ARCH` ausgewählt:

```bash
make ARCH=arm64
```

Für Cross-Compilation zusätzlich:

```bash
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

Dabei bestimmt:

```text
ARCH          -> Kernel-Zielarchitektur
CROSS_COMPILE -> Toolchain-Präfix
```

## 9. Wichtige Targets

```bash
make help
make defconfig
make menuconfig
make oldconfig
make modules
make modules_install
make clean
make mrproper
```

Unterschied:

```text
make clean
    -> entfernt viele Build-Artefakte

make mrproper
    -> entfernt zusätzlich .config und weitere generierte Dateien
```

## 10. Externe Kernelmodule

Auch externe Module nutzen Kbuild.

Beispiel:

```make
obj-m += hello.o

all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
```

Dabei bedeutet:

```text
-C <kernel-build> -> Kernel-Build-Tree verwenden
M=<verzeichnis>   -> externes Modulverzeichnis
```

## 11. Generierte Konfigurationsdaten

Aus `.config` werden weitere Build-Dateien erzeugt, zum Beispiel:

```text
include/generated/autoconf.h
include/config/auto.conf
```

Kernel-Code kann Konfigurationen dadurch direkt verwenden:

```c
#ifdef CONFIG_MY_DRIVER
...
#endif
```

oder:

```c
IS_ENABLED(CONFIG_MY_DRIVER)
```

## 12. `.cmd` und Entwicklungswerkzeuge

Kbuild speichert verwendete Compiler-Kommandos und Abhängigkeiten in Dateien wie:

```text
*.cmd
```

Diese Informationen können beispielsweise von:

```text
scripts/clang-tools/gen_compile_commands.py
```

verwendet werden, um:

```text
compile_commands.json
```

für Werkzeuge wie `clangd` zu erzeugen.

## Typischer Ablauf

```text
          Kconfig
             |
             v
          .config
             |
             v
     generierte Konfiguration
             |
             v
Source ---> Kbuild ---> Compiler / Linker
                         |
              +----------+----------+
              |                     |
              v                     v
           vmlinux               *.ko
              |
              v
       bootbares Kernel-Image
```

Ein typischer Build mit getrenntem Output-Verzeichnis:

```bash
mkdir ../linux-build

make O=../linux-build defconfig
make O=../linux-build menuconfig
make O=../linux-build -j$(nproc)
```

## Merksatz

> **Kconfig beschreibt die auswählbaren Kernel-Funktionen, `.config` enthält die konkrete Auswahl, und Kbuild setzt diese Konfiguration zusammen mit Make, Compiler und Linker in Kernel, Module und weitere Build-Artefakte um.**
