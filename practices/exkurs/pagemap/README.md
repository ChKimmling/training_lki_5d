# Übung: `/proc/<PID>/pagemap` interpretieren

Diese Übung baut auf dem Exkurs zur `/proc/<PID>/pagemap`-Schnittstelle auf und trainiert das Lesen und Interpretieren von Seitenstatusinformationen aus dem Linux-Kernel.

## Lernziel

- die Bedeutung einzelner Bits in einem `pagemap`-Eintrag verstehen
- zwischen virtuellem Adressraum und physischer Seitenabbildung unterscheiden
- Demand Paging anhand von `present`-Bits nachvollziehen
- Soft-Dirty-Bits mit `/proc/self/clear_refs` beobachten
- die Ergebnisse mit `/proc/<PID>/maps` vergleichen

## Vorbereitung

- Linux-System mit einer aktuellen Kernel-Version
- Python 3
- Zugriff auf `/proc/self/pagemap` bzw. `/proc/<PID>/pagemap`
- Keine Root-Rechte nötig, aber die PFN kann aus Sicherheitsgründen ausgeblendet sein

## Material

- Das Skript [pagemap_demo.py](pagemap_demo.py)
- Die ausführliche Theorie im Exkurs [Exkurs_Linux_pagemap_interpretieren.md](Exkurs_Linux_pagemap_interpretieren.md)

## Ablauf

1. Starte das Skript.
2. Beobachte den Zustand direkt nach `mmap()`.
3. Schreibe auf einzelne Seiten und prüfe, welche Einträge `present = 1` setzen.
4. Setze die Soft-Dirty-Bits mit `clear_refs = 4` zurück.
5. Schreibe erneut auf eine zuvor benutzte Seite und prüfe, ob das Soft-Dirty-Bit wieder gesetzt wird.
6. Vergleiche die Adresse mit der Ausgabe aus `/proc/<PID>/maps`.

## Aufgaben

### Aufgabe 1: Initialer Zustand nach `mmap()`

Führe das Skript aus und dokumentiere die Ausgabe unmittelbar nach `mmap()`.

Fragen:

- Welche Seiten sind zu Beginn bereits `present`?
- Was zeigt der Zustand der Seiten 0 bis 3?
- Warum kann eine Seite im reservierten VMA trotzdem `present = 0` sein?

### Aufgabe 2: Schreiben auf einzelne Seiten

Beobachte den Zustand nach dem ersten Schreiben auf Seite 0 und Seite 2.

Fragen:

- Welche Seiten haben nach dem Zugriff das `Present`-Bit gesetzt?
- Warum bleiben Seiten im gleichen VMA möglicherweise unpresentiert?

### Aufgabe 3: Soft-Dirty-Bits analysieren

Nach dem Aufruf von `/proc/self/clear_refs` mit Wert `4` werden die Soft-Dirty-Bits zurückgesetzt.

Fragen:

- Welches Bit markiert Soft-Dirty?
- Was ändert sich unmittelbar nach dem Reset?
- Warum ist ein späterer Schreibzugriff auf dieselbe Seite wieder sichtbar?

### Aufgabe 4: PFN auswerten

Prüfe die Ausgabe der `pagemap`-Einträge und betrachte die PFN-Felder.

Fragen:

- Warum kann die PFN trotz `present = 1` den Wert `0` haben?
- Was bedeutet dies für die Interpretation von `pagemap`-Daten?

### Aufgabe 5: Vergleich mit `/proc/<PID>/maps`

Ergänze die Analyse um die Sicht aus `/proc/<PID>/maps`.

Fragen:

- Welche Information liefert `maps`?
- Welche Information liefert `pagemap`?
- Warum sind beide Ausgaben für das Verständnis von Virtual Memory wichtig?

## Erwartete Beobachtung

Typischer Verlauf, der je nach Kernel, Systemlast und momentaner VM-Statistik leicht abweichen kann:

- direkt nach `mmap()`: die meisten Seiten sind zunächst unpresentiert
- nach Schreiben auf Seite 0 und 2: genau diese Seiten zeigen typischerweise `present = 1`
- Seite 1 und 3 bleiben in der üblichen Beobachtung unpresentiert, solange sie nicht verwendet wurden
- nach `clear_refs = 4`: das Soft-Dirty-Bit wird zurückgesetzt; bei manchen Systemen kann es vor dem Reset bereits gesetzt sein
- nach erneutem Schreiben auf Seite 2: das Soft-Dirty-Bit wird wieder gesetzt

Das entspricht dem Prinzip des Demand Paging: Physische Seiten werden erst bei tatsächlichem Zugriff zugewiesen. Die genaue Bit-Muster hängen aber von der aktuellen Kernel- und Speicherzustandslage ab.

## Musterlösung

### 1. Warum sind nicht alle Seiten präsent?

`mmap()` legt nur den virtuellen Adressraum an. Die zugehörigen Seiten werden nicht automatisch physisch belegt. Erst beim ersten Zugriff erzeugt der Kernel einen Page Fault, reserviert eine physische Seite und setzt das `present`-Bit im PTE.

### 2. Welche Seiten sind nach dem Schreiben präsent?

In der typischen vier-Seiten-Beobachtung sind nur die Seiten präsent, auf die tatsächlich geschrieben wurde; das ist meist Seite 0 und Seite 2. Seite 1 und 3 bleiben unpresentiert, solange sie nicht angesprochen wurden. Die exakte Auswahl kann jedoch systemabhängig leicht variieren.

### 3. Warum kann eine PFN 0 sein?

Auf aktuellen Linux-Systemen wird die PFN aus Sicherheitsgründen häufig nicht vollständig offenbart. Daher kann `present = 1` und `PFN = 0` gleichzeitig auftreten, obwohl die Seite tatsächlich physisch existiert.

### 4. Warum funktioniert `clear_refs = 4`?

`clear_refs = 4` löscht die Soft-Dirty-Bits des Prozesses. Ein späterer Schreibzugriff setzt das Bit erneut, wodurch die Seite als seit dem Reset beschrieben erkannt werden kann. Je nach System kann das Bit vor dem Reset bereits gesetzt sein; der wesentliche Effekt ist aber immer derselbe: eine spätere Änderung setzt das Bit wieder.

### 5. Vergleich mit `/proc/<PID>/maps`

`maps` zeigt die virtuellen Bereiche, also den VMA-Bereich mit Start-, Endadresse und Zugriffsrechten. `pagemap` zeigt für jede virtuelle Seite, ob sie präsent, ausgelagert, Soft-Dirty oder sonst wie gekennzeichnet ist.

## Abschluss

Schreibe in kurzen Stichpunkten die wichtigsten Erkenntnisse dieser Übung auf:

- Demand Paging
- Present-Bit
- Soft-Dirty-Bit
- Unterschied zwischen `maps` und `pagemap`

Wenn du die Ergebnisse gut erklärt hast, bist du in der Lage, die Kern-VM-Laufzeitdaten eines Prozesses auf Seitenebene zu interpretieren.
