# Exkurs: Memory Mappings und Page Faults unter Linux

## Was ist ein Memory Mapping?

Ein **Memory Mapping** beschreibt die Zuordnung eines Bereichs im **virtuellen Adressraum eines Prozesses** zu einer Speicherquelle.

Diese Speicherquelle kann zum Beispiel sein:

- anonymer Speicher, z. B. Heap oder Stack
- eine Datei
- eine Shared Library
- Shared Memory
- ein Device-Mapping

Vereinfacht:

```text
Virtueller Adressraum eines Prozesses
        |
        +--> Code
        +--> Daten
        +--> Heap
        +--> mmap()-Bereiche
        +--> Shared Libraries
        +--> Stack
```

Linux verwendet dafür intern sogenannte **Virtual Memory Areas (VMAs)**.

Ein VMA beschreibt einen zusammenhängenden Bereich mit gleichen Eigenschaften, z. B.:

- Start- und Endadresse
- Zugriffsrechte
- Mapping-Typ
- zugehörige Datei

---

## Virtuelle Adressen statt direkter physischer Adressen

Ein Prozess arbeitet mit **virtuellen Adressen**.

Die MMU der CPU übersetzt diese über Seitentabellen in physische Adressen:

```text
Virtuelle Adresse
       |
       v
Page Table
       |
       v
Physische Adresse
```

Der Prozess sieht also nicht direkt den physischen RAM.

Dadurch erhält jeder Prozess seinen eigenen virtuellen Adressraum.

---

## Memory Mappings anzeigen

Die Mappings eines Prozesses lassen sich anzeigen mit:

```bash
cat /proc/<PID>/maps
```

oder für den aktuellen Prozess:

```bash
cat /proc/self/maps
```

Beispiel:

```text
55c1...-55c1... r-xp ... /usr/bin/bash
7f12...-7f12... r--p ... libc.so.6
7f12...-7f12... rw-p ... [heap]
7ffd...-7ffd... rw-p ... [stack]
```

Die Zugriffsrechte bedeuten:

```text
r = read
w = write
x = execute
p = private
s = shared
```

---

## `mmap()`

Mit dem System Call `mmap()` kann ein Prozess einen neuen Mapping-Bereich erzeugen.

Beispiel:

```c
void *ptr = mmap(NULL,
                 4096,
                 PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS,
                 -1,
                 0);
```

Damit wird ein anonymer Speicherbereich in den virtuellen Adressraum eingeblendet.

Typische Einsatzgebiete:

- dynamischer Speicher
- Shared Libraries
- Memory-Mapped Files
- Shared Memory
- Device Memory

---

## Was ist ein Page Fault?

Ein **Page Fault** entsteht, wenn eine CPU auf eine virtuelle Speicherseite zugreift und die aktuelle Seitentabelle diesen Zugriff nicht unmittelbar auflösen kann.

Beispiel:

```text
Prozess
   |
   | Zugriff auf virtuelle Adresse
   v
MMU
   |
   +--> Mapping vorhanden?
           |
           +--> Ja -> Zugriff
           |
           +--> Nein / Seite nicht präsent
                    |
                    v
               Page Fault
```

Die CPU wechselt daraufhin in den Kernel.

Der Kernel entscheidet, ob der Zugriff gültig ist.

---

## Page Fault bedeutet nicht automatisch Fehler

Ein Page Fault ist ein normaler Bestandteil virtueller Speicherverwaltung.

Beispiel:

```c
char *p = malloc(4096);
```

Der Prozess erhält zunächst virtuellen Adressraum.

Erst beim ersten tatsächlichen Zugriff:

```c
p[0] = 42;
```

kann ein Page Fault auftreten.

Der Kernel:

1. erkennt das gültige Mapping
2. reserviert eine physische Speicherseite
3. aktualisiert die Page Table
4. setzt die Instruktion fort

Dieses Prinzip nennt man:

```text
Demand Paging
```

---

## Minor Page Fault

Ein **Minor Page Fault** kann ohne Zugriff auf ein blockorientiertes Speichermedium behandelt werden.

Beispiele:

- erste Nutzung einer anonymen Speicherseite
- Seite befindet sich bereits im Page Cache
- Copy-on-Write

Ablauf:

```text
Page Fault
   |
   +--> Daten bereits im RAM
   |
   v
Page Table aktualisieren
   |
   v
weiter
```

Minor Faults sind normalerweise relativ günstig.

---

## Major Page Fault

Ein **Major Page Fault** benötigt typischerweise zusätzlichen I/O.

Beispiel:

```text
Prozess greift auf gemappte Datei zu
        |
        v
Seite nicht im RAM
        |
        v
Page Fault
        |
        v
Daten von Storage laden
        |
        v
Page Table aktualisieren
```

Ein Major Fault ist daher deutlich teurer als ein Minor Fault.

---

## Copy-on-Write

Ein typischer Einsatz von Page Faults ist **Copy-on-Write (COW)**.

Nach `fork()` können Parent und Child zunächst dieselben physischen Seiten verwenden:

```text
Parent ----+
           +--> gleiche physische Seite
Child  ----+
```

Die Seiten werden schreibgeschützt markiert.

Wenn einer der Prozesse schreibt:

```text
Write
  |
  v
Page Fault
  |
  v
Seite kopieren
  |
  +--> Parent bekommt Original
  +--> Child bekommt Kopie
```

Dadurch müssen Speicherbereiche nicht sofort vollständig kopiert werden.

---

## Ungültiger Page Fault

Nicht jeder Page Fault kann behandelt werden.

Beispiel:

```c
int *p = NULL;
*p = 42;
```

Der Zugriff erfolgt auf eine Adresse ohne gültiges Mapping.

Der Kernel erkennt:

```text
kein gültiges VMA
```

und sendet dem Prozess typischerweise:

```text
SIGSEGV
```

Daraus entsteht der bekannte:

```text
Segmentation Fault
```

---

## Page Fault und Zugriffsrechte

Auch ein Zugriff auf eine vorhandene Seite kann einen Page Fault auslösen.

Beispiel:

```text
Mapping: read-only
Zugriff: write
```

Dann prüft der Kernel:

```text
Ist Schreiben erlaubt?
```

Falls nein:

```text
SIGSEGV
```

Falls es sich um Copy-on-Write handelt, kann der Kernel stattdessen eine private Kopie erzeugen.

---

## Zusammenhang mit Page Tables

Die Seitentabellen enthalten Informationen wie:

```text
virtuelle Seite
    |
    +--> physische Seite
    +--> present
    +--> read/write
    +--> user/kernel
    +--> accessed
    +--> dirty
```

Ist ein Eintrag nicht präsent oder verletzt der Zugriff die Rechte, löst die CPU einen Page Fault aus.

---

## Page Faults beobachten

Mit `ps`:

```bash
ps -o pid,min_flt,maj_flt,cmd
```

oder mit:

```bash
/usr/bin/time -v ./programm
```

Typische Ausgabe:

```text
Minor page faults: 1234
Major page faults: 4
```

Auch `perf` kann Page Faults messen:

```bash
perf stat -e page-faults,minor-faults,major-faults ./programm
```

---

## Typischer Ablauf

```text
    CPU greift auf virtuelle Adresse zu
                  |
                  v
                 MMU
                  |
           Mapping gültig?
                  |
          +-------+-------+
          |               |
         ja              nein
          |               |
     Seite präsent?    SIGSEGV
          |
     +----+----+
     |         |
    ja        nein
     |         |
  Zugriff   Page Fault
               |
               +--> physische Seite bereitstellen
               +--> Datei laden
               +--> COW durchführen
               |
               v
     Page Table aktualisieren
               |
               v
     Instruktion fortsetzen
```

---

## Merksatz

> **Memory Mappings definieren, welche virtuellen Adressbereiche ein Prozess verwenden darf. Ein Page Fault tritt auf, wenn die aktuelle Speicherübersetzung einen Zugriff nicht direkt erfüllen kann. Viele Page Faults sind normale Mechanismen für Demand Paging und Copy-on-Write; nur ungültige Zugriffe führen typischerweise zu SIGSEGV.**
