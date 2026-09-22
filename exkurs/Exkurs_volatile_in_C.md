# Exkurs: `volatile` in C

## Was bedeutet `volatile`?

Das Schlüsselwort **`volatile`** teilt dem C-Compiler mit:

> Der Wert eines Objekts kann sich außerhalb des normalen Programmflusses ändern.

Der Compiler darf deshalb Zugriffe auf dieses Objekt nicht einfach wegoptimieren oder dauerhaft in einem Register zwischenspeichern.

Beispiel:

```c
volatile int flag;
```

Bei jedem Zugriff auf `flag` muss der Compiler davon ausgehen, dass sich der Wert zwischen zwei Zugriffen geändert haben könnte.

---

## Warum ist das nötig?

Ohne `volatile` darf der Compiler optimieren.

Beispiel:

```c
int flag = 0;

while (flag == 0) {
    // warten
}
```

Der Compiler könnte feststellen:

```text
flag wird in dieser Funktion nie verändert
```

und daraus sinngemäß machen:

```text
flag einmal lesen
       |
       v
immer denselben Wert verwenden
```

Falls `flag` jedoch durch Hardware oder einen anderen externen Mechanismus geändert wird, wäre das problematisch.

Mit:

```c
volatile int flag;
```

muss der Wert bei jedem Schleifendurchlauf erneut gelesen werden.

---

## Typischer Einsatz: Memory-Mapped I/O

In Embedded-Systemen sind Hardware-Register häufig über Speicheradressen erreichbar.

Beispiel:

```c
#define STATUS_REG (*(volatile uint32_t *)0x40000000)
```

Ein Zugriff:

```c
while ((STATUS_REG & 0x01) == 0) {
}
```

muss das Register jedes Mal neu lesen.

Warum?

```text
CPU liest Register
      |
      v
Hardware kann Wert ändern
      |
      v
CPU muss erneut lesen
```

Ohne `volatile` könnte der Compiler den Wert zwischenspeichern.

---

## Lesen und Schreiben

`volatile` betrifft sowohl Lese- als auch Schreibzugriffe.

Beispiel:

```c
volatile uint32_t *reg = ...;

*reg = 1;
*reg = 2;
```

Der Compiler darf diese Schreibzugriffe nicht einfach zu nur einem Zugriff zusammenfassen, wenn beide als beobachtbare `volatile`-Zugriffe gelten.

---

## Was `volatile` NICHT bedeutet

`volatile` ist **kein Synchronisationsmechanismus**.

Es garantiert insbesondere nicht:

- atomare Zugriffe
- Thread-Sicherheit
- gegenseitigen Ausschluss
- Speicherordnung zwischen CPUs
- Cache-Kohärenz
- Schutz vor Race Conditions

Beispiel:

```c
volatile int counter;

counter++;
```

ist **nicht atomar**.

Intern kann das weiterhin sein:

```text
lesen
  |
  v
+1
  |
  v
schreiben
```

Zwei Threads können sich dabei gegenseitig überschreiben.

---

## `volatile` und Threads

Dieses Muster ist für Thread-Synchronisation nicht ausreichend:

```c
volatile int ready = 0;
```

Thread A:

```c
ready = 1;
```

Thread B:

```c
while (!ready) {
}
```

Auch wenn `volatile` erneute Speicherzugriffe erzwingt, löst es nicht die Probleme moderner Mehrkernsysteme bezüglich:

- Race Conditions
- Speicherreihenfolge
- Sichtbarkeit zwischen CPUs

Für Thread-Synchronisation verwendet man stattdessen z. B.:

```text
C11 atomics
Mutex
Semaphore
Spinlock
Condition Variable
```

---

## `volatile` im Linux-Kernel

Im Linux-Kernel gilt ausdrücklich:

> `volatile` ist normalerweise **nicht** das richtige Werkzeug zur Synchronisation.

Der Kernel stellt eigene Mechanismen bereit.

Beispiele:

```text
READ_ONCE()
WRITE_ONCE()
atomic_t
spinlock
mutex
memory barriers
```

Beispiel:

```c
value = READ_ONCE(shared_value);
```

und:

```c
WRITE_ONCE(shared_value, new_value);
```

Diese Makros drücken die gewünschte Semantik gezielter aus als ein pauschales `volatile`.

---

## MMIO im Linux-Kernel

Auch für Hardware-Register sollte Kernel-Code nicht einfach rohe `volatile`-Pointer verwenden.

Stattdessen nutzt Linux Zugriffsfunktionen wie:

```c
readl()
writel()
readb()
writeb()
```

Beispiel:

```c
u32 value = readl(base + STATUS_REG);
writel(value, base + CONTROL_REG);
```

Diese Funktionen berücksichtigen architekturspezifische Anforderungen und Speicherordnung besser als direkter Zugriff über:

```c
volatile uint32_t *
```

---

## `volatile` bei Interrupts

Ein häufiger Embedded-Anwendungsfall ist eine Variable, die von einer ISR verändert wird:

```c
volatile int event_pending = 0;
```

ISR:

```c
event_pending = 1;
```

Main Loop:

```c
if (event_pending) {
    ...
}
```

Hier kann `volatile` nötig sein, damit der Compiler den Wert nicht wegoptimiert.

Aber auch hier gilt:

> `volatile` allein löst keine Probleme mit Atomizität oder Nebenläufigkeit.

---

## Typische sinnvolle Einsatzfälle

`volatile` ist sinnvoll, wenn sich ein Wert außerhalb des normalen C-Programmflusses ändern kann.

Beispiele:

- Memory-Mapped Hardware-Register
- Variablen zwischen Main-Code und ISR in einfachen Embedded-Systemen
- Spezialfälle bei Signal-Handlern
- Low-Level-Code mit expliziten externen Seiteneffekten

Nicht primär für:

- Thread-Synchronisation
- Locks
- Multicore-Kommunikation
- Race-Condition-Vermeidung

---

## `const volatile`

Beide Schlüsselwörter können kombiniert werden:

```c
const volatile uint32_t *status;
```

Das bedeutet:

```text
const
-> Programm darf den Wert nicht schreiben

volatile
-> Wert kann sich trotzdem extern ändern
```

Typisch für ein Hardware-Statusregister:

```text
Software: nur lesen
Hardware: darf Wert verändern
```

---

## Kurzvergleich

| Eigenschaft | `volatile` |
|---|---:|
| erneutes Lesen aus Speicher erzwingen | Ja |
| Schreibzugriffe sichtbar halten | Ja |
| Zugriff atomar machen | Nein |
| Race Conditions verhindern | Nein |
| Threads synchronisieren | Nein |
| Memory Barrier ersetzen | Nein |
| für MMIO grundsätzlich relevant | Ja |
| im Linux-Kernel für Synchronisation empfohlen | Nein |

---

## Merksatz

> **`volatile` sagt dem Compiler: „Dieser Wert kann sich außerhalb des normalen Programmflusses ändern – führe die Speicherzugriffe tatsächlich aus.“ Es sagt jedoch nichts über Atomizität, Synchronisation oder Speicherordnung aus.**
