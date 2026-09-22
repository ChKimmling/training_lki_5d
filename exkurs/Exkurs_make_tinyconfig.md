# Exkurs: `make tinyconfig` beim Linux-Kernel

## Ziel

`make tinyconfig` erzeugt eine **möglichst kleine Kernel-Konfiguration**. Das Kconfig-Buildsystem beschreibt das Target als „Configure the tiniest possible kernel“.

```text
Kconfig
   |
   +--> allnoconfig
   +--> tiny-base.config
   +--> tiny.config
   |
   v
.config
```

## 1. Kernel-Quellen holen

```bash
git clone https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
cd linux
```

Optional einen bestimmten Stand auschecken:

```bash
git checkout v6.18
```

## 2. Tiny-Konfiguration erzeugen

Für die Host-Architektur:

```bash
make tinyconfig
```

Explizit für x86-64:

```bash
make ARCH=x86_64 tinyconfig
```

Das Ergebnis liegt anschließend in:

```text
.config
```

Aktive Optionen anzeigen:

```bash
grep '=y' .config
grep '=m' .config
```

## 3. Was aktiviert `tinyconfig` zusätzlich?

Das aktuelle `kernel/configs/tiny.config` aktiviert nur wenige gezielte Optionen:

```text
CONFIG_CC_OPTIMIZE_FOR_SIZE=y
CONFIG_KERNEL_XZ=y
CONFIG_SLUB=y
CONFIG_SLUB_TINY=y
CONFIG_LD_DEAD_CODE_DATA_ELIMINATION=y
```

Damit wird u. a.:

- auf kleine Codegröße optimiert,
- das Kernel-Image mit XZ komprimiert,
- SLUB als Objekt-Allocator verwendet,
- `SLUB_TINY` aktiviert,
- unbenutzter Code und unbenutzte Daten beim Linken entfernt.

## 4. Welche Subsysteme bleiben enthalten?

`tinyconfig` ist **keine feste Subsystemliste**. Das Ergebnis hängt von Architektur, Kernel-Version, Compiler und Kconfig-Abhängigkeiten ab.

Enthalten bleiben vor allem unverzichtbare Kernmechanismen, z. B.:

```text
Kernel Core
├── Task-/Prozessverwaltung
├── Scheduler-Grundfunktionen
├── Interrupt-/Exception-Basis
├── virtuelle Speicherverwaltung
├── Page Allocator
├── SLUB / SLUB_TINY
├── grundlegendes VFS
├── System-Call-Infrastruktur
└── architekturspezifischer Basiscode
```

Viele optionale Bereiche sind dagegen typischerweise deaktiviert:

```text
Netzwerk
USB
Bluetooth
WLAN
Sound
DRM/Grafik
viele Dateisysteme
Tracing/Debugging
Virtualisierung
zusätzliche Gerätetreiber
```

## 5. Enthält `tinyconfig` Kernelmodule?

Typischerweise ist:

```text
CONFIG_MODULES=n
```

Prüfen:

```bash
grep CONFIG_MODULES .config
```

Typische Ausgabe:

```text
# CONFIG_MODULES is not set
```

Damit werden keine klassischen ladbaren Kernelmodule (`*.ko`) gebaut. Aktive Funktionen werden direkt in den Kernel eingebaut.

## 6. Kernel bauen

```bash
make -j$(nproc)
```

Auf x86 entstehen typischerweise:

```text
vmlinux
arch/x86/boot/bzImage
```

Größe prüfen:

```bash
ls -lh vmlinux arch/x86/boot/bzImage
```

## 7. Ist `tinyconfig` direkt bootfähig?

Nicht unbedingt.

Das Ziel lautet:

```text
kleinstmögliche Konfiguration
```

und nicht:

```text
kleinstmöglicher sofort nutzbarer PC-/QEMU-Kernel
```

Für einen praktikablen QEMU- oder x86-Kernel müssen häufig gezielt ergänzt werden:

```text
Serielle Konsole
PCI
VirtIO
Block-Device-Support
Root-Dateisystem
devtmpfs
initramfs
```

Typischer Ablauf:

```bash
make tinyconfig
make menuconfig
make -j$(nproc)
```

## 8. `tinyconfig` vs. `defconfig`

```text
make defconfig
    -> sinnvoller Architektur-Standardkernel

make tinyconfig
    -> möglichst kleiner Kernel als Ausgangspunkt
```

## Praktischer Analyseablauf

```bash
git clone https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
cd linux

make ARCH=x86_64 tinyconfig

grep '=y' .config
grep '=m' .config
grep -E 'CONFIG_(MODULES|NET|USB|PCI|EXT4_FS|BLOCK|SMP|SLUB)' .config

make -j$(nproc)
```

## Merksatz

> **`make tinyconfig` erzeugt nicht den kleinsten direkt nutzbaren Linux-Kernel, sondern die kleinstmögliche Kconfig-Auswahl für die jeweilige Architektur; optionale Treiber und Subsysteme sind weitgehend deaktiviert und Loadable Kernel Modules typischerweise ausgeschaltet.**
