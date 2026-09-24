# Exkurs: `/proc/<PID>/maps` und `/proc/<PID>/pagemap`

## Zwei Sichtweisen auf den virtuellen Speicher

Linux stellt über `procfs` verschiedene Sichten auf den Adressraum eines Prozesses bereit:

| Schnittstelle | Sicht | Kernaussage |
|---|---|---|
| `/proc/<PID>/maps` | Virtuelle Speicherbereiche (VMAs) | Welche Adressbereiche existieren und wie dürfen sie verwendet werden? |
| `/proc/<PID>/pagemap` | Einzelne virtuelle Seiten | Ist eine konkrete Seite präsent, ausgelagert oder besonders markiert? |

`maps` beschreibt damit die **logische Aufteilung des virtuellen Adressraums**. `pagemap` liefert eine **seitenweise Momentaufnahme der Abbildung**. Keine der beiden Dateien ist eine unveränderte Ausgabe der Hardware-Seitentabellen.

---

## `/proc/<PID>/maps`: Sicht auf die VMAs

Jede Zeile beschreibt einen zusammenhängenden virtuellen Speicherbereich mit gleichen Eigenschaften:

```text
55d7e8c92000-55d7e8c93000 r--p 00000000 08:01 123456 /usr/bin/demo
55d7e8c93000-55d7e8c94000 r-xp 00001000 08:01 123456 /usr/bin/demo
7ffd6f31d000-7ffd6f33e000 rw-p 00000000 00:00      0 [stack]
```

```text
Adresse                  Rechte Offset   Gerät Inode  Pfad
55d7e8c93000-55d7e8c94000 r-xp 00001000 08:01 123456 /usr/bin/demo
```

| Feld | Bedeutung |
|---|---|
| Adresse | Anfang und Ende des virtuellen Bereichs; das Ende gehört nicht mehr dazu |
| Rechte | `r` lesen, `w` schreiben, `x` ausführen |
| `p` / `s` | privates Copy-on-Write- beziehungsweise Shared-Mapping |
| Offset | Position innerhalb der abgebildeten Datei |
| Gerät | Major- und Minor-Nummer des Dateisystems |
| Inode | Inode der abgebildeten Datei |
| Pfad | Datei oder Bezeichnung wie `[heap]`, `[stack]` oder `[vdso]` |

Ein Eintrag in `maps` bedeutet **nicht**, dass bereits jede Seite dieses Bereichs im RAM liegt. Ein VMA beschreibt zunächst nur, dass Zugriffe auf diesen Bereich grundsätzlich zulässig sind und wie der Kernel sie behandeln soll.

---

## `/proc/<PID>/pagemap`: Sicht auf einzelne Seiten

`pagemap` ist eine Binärdatei. Für jede virtuelle Seite enthält sie einen 64 Bit breiten Eintrag. Bei einer Seitengröße von 4096 Byte wird der zu einer virtuellen Adresse gehörende Eintrag so gefunden:

```text
virtuelle Seitennummer = virtuelle Adresse / 4096
pagemap-Dateioffset    = virtuelle Seitennummer × 8
```

Wichtige Bits eines Eintrags:

| Bits | Bedeutung |
|---:|---|
| 0–54 | Page Frame Number (PFN), falls die Seite präsent ist |
| 0–4 | Swap-Typ, falls die Seite ausgelagert ist |
| 5–54 | Swap-Offset, falls die Seite ausgelagert ist |
| 55 | Soft-Dirty: seit dem letzten Reset beschrieben |
| 56 | Seite wird als exklusiv abgebildet gemeldet |
| 57 | `userfaultfd` Write-Protect (`uffd-wp`) |
| 58 | Guard Region, seit Linux 6.15 |
| 61 | Datei-Mapping oder gemeinsam genutztes anonymes Mapping |
| 62 | Seite ist ausgelagert (`swapped`) |
| 63 | Seite ist im RAM präsent (`present`) |

Die PFN ist seit Linux 4.2 ohne `CAP_SYS_ADMIN` normalerweise auf `0` gesetzt. Die Statusbits können dennoch ausgewertet werden. Hintergrund ist, dass physische Adressinformationen Angriffe wie Rowhammer erleichtern können.

---

## Zusammenhang zwischen VMA und Page Table

```text
mmap(), exec() oder Bibliothekslader
                │
                ▼
       VMA wird angelegt
       sichtbar in maps
                │
      erster Speicherzugriff
                ▼
          Page Fault
                │
                ▼
 physische Seite wird bereitgestellt
 Page-Table-Eintrag wird aktualisiert
                │
                ▼
   Seite erscheint in pagemap
       typischerweise present = 1
```

Ein Bereich kann daher vollständig in `maps` erscheinen, während nur ein Teil seiner Seiten in `pagemap` als präsent markiert ist. Das ist normales **Demand Paging**.

### Beispiel

Ein Prozess reserviert vier Seiten mit `mmap()`, greift aber nur auf Seite 0 und Seite 2 zu:

| Seite | In `maps` enthalten | `pagemap`: Present |
|---:|:---:|:---:|
| 0 | ja | 1 |
| 1 | ja | 0 |
| 2 | ja | 1 |
| 3 | ja | 0 |

`maps` zeigt den gesamten reservierten Bereich. `pagemap` macht sichtbar, welche Seiten davon aktuell physisch abgebildet sind.

---

## Beispiel für die Interpretation

Angenommen, ein `pagemap`-Eintrag lautet:

```text
0x8180000000000000
```

Dann ergibt die Bitprüfung:

```text
Bit 63 = 1  → Seite ist präsent
Bit 62 = 0  → Seite ist nicht ausgelagert
Bit 61 = 0  → kein Datei- oder Shared-Anon-Mapping
Bit 56 = 1  → Seite wird als exklusiv gemeldet
Bit 55 = 1  → Soft-Dirty ist gesetzt
PFN    = 0  → möglicherweise mangels CAP_SYS_ADMIN ausgeblendet
```

Bei sichtbarer PFN ließe sich die physische Adresse berechnen:

```text
physische Adresse = PFN × Seitengröße + Offset innerhalb der Seite
```

Diese Zuordnung ist nur eine Momentaufnahme. Copy-on-Write, Reclaim, Swap, Page Migration oder THP-Splitting können sie verändern.

---

## Grenzen und typische Fallstricke

- `pagemap` liefert keinen vollständigen Roh-PTE und zeigt nicht alle architekturspezifischen PTE-Bits.
- Ein VMA in `maps` garantiert keine residente physische Seite.
- `present = 0` kann eine noch nicht angeforderte, ausgelagerte oder nicht abgebildete Seite bedeuten; zur Unterscheidung müssen weitere Bits geprüft werden.
- Die Dateien werden während der Auswertung nicht automatisch eingefroren. Der Prozess kann seinen Adressraum parallel verändern.
- Huge Pages können die Interpretation von PFN- und Exclusive-Informationen beeinflussen.
- Zugriffe auf fremde Prozesse unterliegen den `ptrace`-Berechtigungsprüfungen des Kernels.

## Ergänzende Schnittstellen

| Schnittstelle | Zweck |
|---|---|
| `/proc/<PID>/smaps` | VMAs plus RSS, PSS sowie private und gemeinsam genutzte Seiten |
| `/proc/<PID>/smaps_rollup` | Zusammengefasste Speicherstatistik des Prozesses |
| `/proc/<PID>/clear_refs` | Unter anderem Soft-Dirty-Bits zurücksetzen |
| `/proc/kpageflags` | Eigenschaften physischer Seiten, indiziert über die PFN |
| `/proc/kpagecount` | Anzahl der Abbildungen einer physischen Seite |

## Merksatz

> **`maps` zeigt, welcher virtuelle Speicherbereich existiert und welche Regeln dort gelten. `pagemap` zeigt seitenweise, ob und wie dieser Bereich aktuell abgebildet ist.**

## Quellen

- Linux-Kernel-Dokumentation: [The `/proc` Filesystem](https://docs.kernel.org/filesystems/proc.html)
- Linux-Kernel-Dokumentation: [Examining Process Page Tables](https://docs.kernel.org/admin-guide/mm/pagemap.html)
