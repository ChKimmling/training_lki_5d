# Exkurs: Jiffies im Linux-Kernel

## Was sind Jiffies?

**Jiffies** sind eine interne Zeiteinheit des Linux-Kernels.

Sie zählen vereinfacht, wie viele **Kernel-Ticks** seit dem Systemstart vergangen sind.

```text
Systemstart
   |
   v
jiffies = 0
   |
   +--> Tick
   |
   v
jiffies = 1
   |
   +--> Tick
   |
   v
jiffies = 2
```

Die globale Variable heißt:

```c
jiffies
```

und wird vom Kernel für viele zeitbezogene Aufgaben verwendet.

---

## Zusammenhang mit `HZ`

Wie viel Zeit ein Jiffy repräsentiert, hängt von:

```text
CONFIG_HZ
```

ab.

Typische Werte sind zum Beispiel:

```text
HZ = 100
HZ = 250
HZ = 1000
```

Die Dauer eines Jiffies ist:

```text
1 / HZ Sekunden
```

Beispiele:

```text
HZ = 100   -> 1 Jiffy = 10 ms
HZ = 250   -> 1 Jiffy = 4 ms
HZ = 1000  -> 1 Jiffy = 1 ms
```

Wichtig:

> Ein Jiffy ist keine feste Zeitspanne über alle Linux-Systeme hinweg.

---

## Beispiel

Bei:

```text
HZ = 250
```

entsprechen:

```text
250 Jiffies = 1 Sekunde
500 Jiffies = 2 Sekunden
25 Jiffies  = 100 ms
```

Für Umrechnungen sollte Kernel-Code jedoch nicht selbst rechnen, sondern Hilfsfunktionen verwenden.

---

## Wichtige Umrechnungsfunktionen

Millisekunden nach Jiffies:

```c
msecs_to_jiffies(500)
```

Mikrosekunden nach Jiffies:

```c
usecs_to_jiffies(1000)
```

Jiffies nach Millisekunden:

```c
jiffies_to_msecs(jiffies)
```

Beispiel:

```c
unsigned long timeout;

timeout = jiffies + msecs_to_jiffies(500);
```

Damit wird ein Zeitpunkt 500 ms in der Zukunft beschrieben.

---

## Typisches Timeout-Muster

Ein häufiges Kernel-Muster lautet:

```c
unsigned long timeout =
    jiffies + msecs_to_jiffies(1000);
```

Später wird geprüft:

```c
if (time_after(jiffies, timeout)) {
    /* Timeout erreicht */
}
```

oder:

```c
if (time_before(jiffies, timeout)) {
    /* Timeout noch nicht erreicht */
}
```

---

## Warum nicht einfach vergleichen?

Naiv könnte man schreiben:

```c
if (jiffies > timeout)
```

Das ist problematisch, weil der Jiffies-Zähler irgendwann überläuft.

Deshalb stellt Linux spezielle Makros bereit:

```c
time_after()
time_before()
time_after_eq()
time_before_eq()
```

Diese berücksichtigen den Überlauf korrekt.

---

## Jiffies-Overflow

Da `jiffies` ein Integer-Zähler ist, läuft er irgendwann über.

Beispiel vereinfacht:

```text
... 0xFFFFFFFE
... 0xFFFFFFFF
... 0x00000000
... 0x00000001
```

Deshalb sind direkte Vergleiche riskant.

Richtig:

```c
if (time_after(jiffies, deadline))
```

Falsch:

```c
if (jiffies > deadline)
```

---

## `jiffies` vs. `jiffies_64`

Linux stellt zusätzlich einen 64-Bit-Zähler bereit:

```c
jiffies_64
```

Auf 64-Bit-Systemen ist die praktische Overflow-Zeit extrem groß.

Auf 32-Bit-Systemen ist der Überlauf von `jiffies` deutlich relevanter.

Für portable Kernel-Logik sollten trotzdem die vorgesehenen Zeitmakros verwendet werden.

---

## Jiffies und Timer

Kernel-Timer arbeiten häufig mit Jiffies.

Beispiel:

```c
mod_timer(&my_timer,
          jiffies + msecs_to_jiffies(1000));
```

Das bedeutet:

```text
Timer soll in ca. 1 Sekunde auslösen
```

Typischer Ablauf:

```text
jiffies jetzt
    |
    +--> + msecs_to_jiffies(1000)
    |
    v
Ablaufzeitpunkt
```

---

## Jiffies und Schlafen

Auch Wartefunktionen verwenden häufig Jiffies oder darauf basierende Umrechnungen.

Beispiel:

```c
schedule_timeout(msecs_to_jiffies(100));
```

Der aktuelle Task schläft dabei ungefähr 100 ms.

Je nach Mechanismus und Scheduler ist das keine Garantie für eine exakt mikrosekundengenaue Wartezeit.

---

## Tickless Kernel

Moderne Linux-Systeme verwenden häufig:

```text
CONFIG_NO_HZ
```

also einen sogenannten **tickless Kernel**.

Das bedeutet:

> Der Kernel muss nicht auf jeder CPU permanent periodische Timer-Interrupts erzeugen.

Trotzdem bleibt `jiffies` als logische Kernel-Zeitbasis erhalten.

Es ist daher falsch zu denken:

```text
1 Jiffy = exakt ein Hardware-Timer-Interrupt
```

auf jedem modernen System.

Vielmehr ist `jiffies` eine logische Tick-basierte Zeitrepräsentation.

---

## Jiffies vs. hochauflösende Zeit

Jiffies eignen sich gut für:

- Timeouts
- einfache Kernel-Timer
- relative Zeitangaben
- Scheduler-nahe Zeitlogik

Für hochauflösende Zeitmessung gibt es andere Mechanismen:

```text
ktime_t
ktime_get()
hrtimer
clocksource
```

Beispiel:

```c
ktime_t now = ktime_get();
```

Diese Mechanismen arbeiten typischerweise wesentlich feiner als Jiffies.

---

## Typisches Beispiel

```c
unsigned long deadline;

deadline = jiffies + msecs_to_jiffies(250);

while (!condition) {
    if (time_after_eq(jiffies, deadline))
        return -ETIMEDOUT;
}
```

Bedeutung:

```text
jetzt
 |
 +--> 250 ms warten
 |
 +--> Bedingung erfüllt?
 |        |
 |        +--> ja -> weiter
 |
 +--> Deadline erreicht?
          |
          +--> ja -> Timeout
```

---

## Wichtige Helfer

```text
msecs_to_jiffies()
usecs_to_jiffies()

jiffies_to_msecs()
jiffies_to_usecs()

time_after()
time_before()
time_after_eq()
time_before_eq()
```

Diese Funktionen und Makros sollten gegenüber manuellen Berechnungen bevorzugt werden.

---

## Gesamtbild

```text
CONFIG_HZ
   |
   v
definiert Tick-Auflösung
   |
   v
jiffies
   |
   +--> Timeouts
   +--> Kernel-Timer
   +--> Wartezeiten
   +--> Scheduler-nahe Logik
```

---

## Merksatz

> **`jiffies` ist die klassische tick-basierte Zeitbasis des Linux-Kernels. Die konkrete Zeit pro Jiffy hängt von `HZ` ab; für Umrechnungen und Vergleiche sollten immer die Kernel-Hilfsfunktionen und `time_*`-Makros verwendet werden.**
