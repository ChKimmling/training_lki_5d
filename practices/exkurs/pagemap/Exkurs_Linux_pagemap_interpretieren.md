# Exkurs: `/proc/<PID>/pagemap` interpretieren

## Ziel

`/proc/<PID>/pagemap` ermöglicht eine seitenweise Sicht auf den virtuellen Adressraum eines Prozesses. In dieser kurzen Übung werden vier anonyme Speicherseiten reserviert, einzelne Seiten beschrieben und anschließend ihre `pagemap`-Einträge ausgewertet.

**Dauer:** ca. 30 Minuten  
**Voraussetzungen:** Linux, Python 3, grundlegendes Verständnis von VMAs, PTEs und Page Faults

> **Wichtig:** `/proc/<PID>/maps` zeigt die virtuellen Speicherbereiche (VMAs). `pagemap` liefert dagegen Informationen zu einzelnen virtuellen Seiten. Es bildet einen PTE jedoch nicht unverändert ab, sondern stellt eine definierte Kernel-Schnittstelle mit ausgewählten Informationen bereit.

## Aufbau eines `pagemap`-Eintrags

Für jede virtuelle Seite enthält `pagemap` einen 64 Bit breiten Eintrag:

| Bits | Bedeutung |
|---:|---|
| 0–54 | Page Frame Number (PFN), wenn die Seite präsent ist |
| 0–4 | Swap-Typ, wenn die Seite ausgelagert ist |
| 5–54 | Swap-Offset, wenn die Seite ausgelagert ist |
| 55 | Soft-Dirty |
| 56 | Seite exklusiv abgebildet |
| 57 | Durch `userfaultfd` schreibgeschützt (`uffd-wp`) |
| 61 | Datei-Mapping oder gemeinsam genutztes anonymes Mapping |
| 62 | Seite ist ausgelagert (`swapped`) |
| 63 | Seite ist im RAM präsent (`present`) |

Die Position eines Eintrags wird aus der virtuellen Adresse berechnet:

```text
virtuelle Seitennummer = virtuelle Adresse / Seitengröße
Datei-Offset           = virtuelle Seitennummer × 8 Byte
```

Die PFN ist auf aktuellen Linux-Systemen aus Sicherheitsgründen normalerweise nur mit `CAP_SYS_ADMIN` sichtbar. Ohne diese Berechtigung kann sie trotz `present = 1` als `0` erscheinen.

## Versuchsskript

Speichere das folgende Programm als `pagemap_demo.py`:

```python
#!/usr/bin/env python3

import ctypes
import mmap
import os
import sys

PAGE_SIZE = os.sysconf("SC_PAGE_SIZE")
PAGE_COUNT = 4
PFN_MASK = (1 << 55) - 1

memory = mmap.mmap(
    -1,
    PAGE_COUNT * PAGE_SIZE,
    flags=mmap.MAP_PRIVATE | mmap.MAP_ANONYMOUS,
    prot=mmap.PROT_READ | mmap.PROT_WRITE,
)

base_address = ctypes.addressof(ctypes.c_char.from_buffer(memory))


def read_pagemap(address):
    page_number = address // PAGE_SIZE
    file_offset = page_number * 8

    with open("/proc/self/pagemap", "rb", buffering=0) as pagemap:
        pagemap.seek(file_offset)
        raw = pagemap.read(8)

    if len(raw) != 8:
        raise RuntimeError("pagemap-Eintrag konnte nicht gelesen werden")

    return int.from_bytes(raw, byteorder=sys.byteorder)


def show_pages(title):
    print(f"\n{title}")
    print("Seite  virtuelle Adresse   Eintrag             P S F X D PFN")

    for page in range(PAGE_COUNT):
        address = base_address + page * PAGE_SIZE
        entry = read_pagemap(address)

        present = (entry >> 63) & 1
        swapped = (entry >> 62) & 1
        file_page = (entry >> 61) & 1
        exclusive = (entry >> 56) & 1
        soft_dirty = (entry >> 55) & 1
        pfn = entry & PFN_MASK

        print(
            f"{page:5}  0x{address:016x} "
            f"0x{entry:016x}  "
            f"{present} {swapped} {file_page} "
            f"{exclusive} {soft_dirty} 0x{pfn:x}"
        )


print(f"PID:          {os.getpid()}")
print(f"Seitengröße:  {PAGE_SIZE} Byte")
print(f"Startadresse: 0x{base_address:x}")

show_pages("1. Direkt nach mmap()")

memory[0 * PAGE_SIZE] = 0x11
memory[2 * PAGE_SIZE] = 0x22

show_pages("2. Nach dem Schreiben auf Seite 0 und 2")

with open("/proc/self/clear_refs", "w") as clear_refs:
    clear_refs.write("4\n")

show_pages("3. Nach dem Löschen der Soft-Dirty-Bits")

memory[2 * PAGE_SIZE] = 0x33

show_pages("4. Nach erneutem Schreiben auf Seite 2")
```

Ausführen:

```bash
python3 pagemap_demo.py
```

Legende der Ausgabe:

| Kürzel | Bedeutung |
|:---:|---|
| P | Present |
| S | Swapped |
| F | File-Page oder Shared-Anon |
| X | Exclusive |
| D | Soft-Dirty |

## Arbeitsaufträge

1. Vergleiche die Einträge direkt nach `mmap()` mit den Einträgen nach dem ersten Schreibzugriff.
2. Welche Seiten besitzen danach das `Present`-Bit?
3. Warum können die Seiten 1 und 3 weiterhin `Present = 0` zeigen, obwohl sie innerhalb des mit `mmap()` reservierten Bereichs liegen?
4. Beobachte Bit 55 vor und nach dem Schreiben von `4` nach `/proc/self/clear_refs`.
5. Welche Seite erhält nach dem erneuten Schreibzugriff wieder das Soft-Dirty-Bit?
6. Warum kann die PFN trotz `Present = 1` den Wert `0` besitzen?
7. Vergleiche die Startadresse zusätzlich mit dem passenden Eintrag in `/proc/<PID>/maps`.

## Erwartete Beobachtung

`mmap()` erzeugt zunächst einen virtuellen Speicherbereich. Die zugehörigen physischen Seiten müssen noch nicht vorhanden sein:

```text
mmap()
   ↓
VMA wird angelegt
   ↓
Seiten sind zunächst möglicherweise nicht präsent
   ↓
erster Zugriff erzeugt einen Page Fault
   ↓
Kernel beschafft eine physische Seite und aktualisiert die Page Table
```

Nach dem Schreiben auf Seite 0 und 2 ist typischerweise zu beobachten:

| Seite | Erwartetes Ergebnis |
|---:|---|
| 0 | `present = 1` |
| 1 | `present = 0` |
| 2 | `present = 1` |
| 3 | `present = 0` |

Das Ergebnis demonstriert **Demand Paging**: Ein vorhandener VMA bedeutet nicht automatisch, dass bereits für jede virtuelle Seite eine physische Seite und ein präsent markierter PTE existieren.

Durch `clear_refs = 4` löscht der Kernel die Soft-Dirty-Markierungen des Prozesses. Ein anschließender Schreibzugriff auf Seite 2 setzt deren Soft-Dirty-Bit erneut. Damit lassen sich Seiten erkennen, die seit dem Zurücksetzen beschrieben wurden.

## Beispielinterpretation

Ein Eintrag mit dem Wert

```text
0x8180000000000000
```

wird wie folgt interpretiert:

```text
Bit 63 = 1  → Seite ist präsent
Bit 62 = 0  → Seite ist nicht ausgelagert
Bit 61 = 0  → kein Datei- oder Shared-Anon-Mapping
Bit 56 = 1  → Seite wird als exklusiv abgebildet gemeldet
Bit 55 = 1  → Soft-Dirty ist gesetzt
PFN    = 0  → wahrscheinlich mangels Berechtigung ausgeblendet
```

Die physische Adresse könnte bei sichtbarer PFN grundsätzlich folgendermaßen berechnet werden:

```text
physische Adresse = PFN × Seitengröße + Offset innerhalb der Seite
```

Diese Adresse ist jedoch nur eine Momentaufnahme: Page Migration, Reclaim, Swap oder Copy-on-Write können die Abbildung anschließend verändern.

## Musterlösung

- `maps` weist den gesamten durch `mmap()` reservierten Bereich aus.
- Nach dem gezielten Schreiben sind normalerweise nur Seite 0 und Seite 2 präsent.
- Ursache ist die verzögerte physische Speicherzuweisung durch Demand Paging.
- `clear_refs = 4` löscht die Soft-Dirty-Bits und aktiviert die Erkennung nachfolgender Schreibzugriffe.
- Nach dem erneuten Schreiben wird Seite 2 wieder als Soft-Dirty gemeldet.
- Eine PFN von `0` bedeutet bei einer präsent markierten Seite nicht zwingend Page Frame 0; häufig wurde die PFN aus Sicherheitsgründen ausgeblendet.

## Merksatz

> `/proc/<PID>/maps` zeigt, **welche virtuellen Bereiche existieren**. `/proc/<PID>/pagemap` zeigt seitenweise, **welche dieser Seiten aktuell präsent, ausgelagert oder seit einem Reset beschrieben wurden**.

## Weiterführende Schnittstellen

| Schnittstelle | Zweck |
|---|---|
| `/proc/<PID>/maps` | VMAs und Zugriffsrechte anzeigen |
| `/proc/<PID>/smaps` | VMAs mit RSS-, PSS- und weiteren Statistiken anzeigen |
| `/proc/<PID>/pagemap` | Seitenweise Mapping-Informationen lesen |
| `/proc/kpageflags` | Eigenschaften einer physischen Seite über ihre PFN ermitteln |
| `/proc/kpagecount` | Anzahl der Abbildungen einer physischen Seite ermitteln |

## Quelle

- Linux-Kernel-Dokumentation: `Documentation/admin-guide/mm/pagemap.rst`
- Online: <https://docs.kernel.org/admin-guide/mm/pagemap.html>
