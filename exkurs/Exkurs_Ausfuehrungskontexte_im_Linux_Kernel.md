# Exkurs: Ausführungskontexte im Linux-Kernel

## Warum sind Ausführungskontexte wichtig?

Kernel-Code läuft nicht immer unter denselben Bedingungen. Entscheidend ist, **in welchem Kontext** eine Funktion ausgeführt wird.

Davon hängt unter anderem ab:

- ob der Code schlafen darf
- ob blockierende Operationen erlaubt sind
- welche Synchronisationsmechanismen verwendet werden dürfen
- wie lange eine Funktion laufen sollte

Die wichtigste Unterscheidung lautet:

```text
Kernel-Ausführung
       |
       +--> Prozesskontext
       |
       +--> Interruptkontext
```

---

## Prozesskontext

Kernel-Code läuft im **Prozesskontext**, wenn er im Namen eines Prozesses oder Kernel-Threads ausgeführt wird.

Typische Beispiele:

- System Calls
- Treiberfunktionen wie `read()` oder `write()`
- Kernel-Threads
- Workqueues
- Threaded IRQs

Beispiel:

```text
Userspace-Prozess
       |
       | read()
       v
System Call
       |
       v
Kernel-Code
```

Im Prozesskontext existiert ein aktueller Prozess:

```c
current
```

Da dieser vom Scheduler verwaltet wird, darf Kernel-Code hier grundsätzlich:

- schlafen
- auf I/O warten
- Mutexes verwenden
- blockierende Operationen ausführen

Wichtig: Ob eine konkrete Funktion schlafen darf, hängt zusätzlich davon ab, ob sie gerade in einem atomaren Abschnitt ausgeführt wird.

---

## Hard-IRQ-Kontext

Ein Hardware-Interrupt kann die aktuelle CPU-Ausführung asynchron unterbrechen.

```text
Prozess läuft
     |
     +---- Hardware Interrupt
                 |
                 v
            IRQ Handler
```

Der IRQ-Handler läuft im **Hard-IRQ-Kontext**.

Hier gelten strenge Regeln:

- nicht schlafen
- nicht blockieren
- keine normalen Mutexes verwenden
- keine lang laufenden Operationen durchführen
- keine Operationen verwenden, die schlafen könnten

Der Handler sollte daher möglichst kurz bleiben.

Typische Aufgaben:

- Interruptquelle bestätigen
- Statusregister lesen
- Daten sichern
- weitere Verarbeitung an späteren Kontext delegieren

---

## SoftIRQ-Kontext

Aufwendigere Arbeiten werden häufig aus dem Hard-IRQ heraus verschoben.

Dafür stellt Linux unter anderem **SoftIRQs** bereit.

```text
Hardware
   |
   v
Hard IRQ
   |
   | kurze Sofortarbeit
   v
SoftIRQ
```

SoftIRQs laufen ebenfalls in einem **atomaren Kontext**.

Das bedeutet:

```text
Schlafen:        nein
Blockieren:      nein
Mutex verwenden: nein
```

Typische Einsatzbereiche:

- Netzwerk RX/TX
- Timer
- RCU

---

## Tasklets

Tasklets basieren intern auf SoftIRQs und wurden lange in Gerätetreibern verwendet.

Auch sie laufen in einem nicht schlafbaren Kontext.

```text
Hard IRQ
   |
   v
Tasklet
```

Für neue Treiber werden heute häufig andere Mechanismen bevorzugt, zum Beispiel:

- Workqueues
- Threaded IRQs

---

## Workqueues

Eine **Workqueue** verschiebt Arbeit in einen Kernel-Worker-Thread.

```text
IRQ
 |
 | Work einplanen
 v
kworker Thread
 |
 v
Workqueue-Funktion
```

Da ein echter Kernel-Thread die Funktion ausführt, läuft die Workqueue im **Prozesskontext**.

Damit sind grundsätzlich möglich:

- schlafen
- Mutexes verwenden
- auf I/O warten
- aufwendigere Verarbeitung durchführen

Workqueues eignen sich daher gut für Arbeit, die nicht direkt im Interrupt-Handler erledigt werden muss.

---

## Threaded IRQ

Linux unterstützt Interrupt-Handler, deren Hauptarbeit in einem Kernel-Thread ausgeführt wird.

Typischer Aufbau:

```text
Hardware IRQ
      |
      v
kurzer Primary Handler
      |
      v
IRQ Thread
```

Ein solcher Handler kann beispielsweise mit:

```c
request_threaded_irq()
```

registriert werden.

Der Thread-Anteil läuft im Prozesskontext und darf daher deutlich mehr tun als ein klassischer Hard-IRQ-Handler.

---

## Übersicht

| Kontext | Beispiel | Schlafen erlaubt? |
|---|---|---:|
| Prozesskontext | System Call | Ja |
| Prozesskontext | Kernel-Thread | Ja |
| Prozesskontext | Workqueue | Ja |
| Prozesskontext | Threaded IRQ | Ja |
| Hard-IRQ-Kontext | klassischer IRQ-Handler | Nein |
| SoftIRQ-Kontext | Netzwerk, Timer | Nein |
| Tasklet-Kontext | Deferred Work | Nein |

---

## Typischer Ablauf in einem Treiber

```text
Hardware Interrupt
        |
        v
Hard IRQ Handler
        |
        | kurze, zeitkritische Arbeit
        v
Workqueue oder Threaded IRQ
        |
        | darf schlafen
        v
aufwendige Verarbeitung
```

Dieses Muster verhindert, dass lange Interrupt-Handler andere Kernel-Aktivitäten unnötig blockieren.

---

## Atomarer Kontext

Ein wichtiger Oberbegriff ist der **atomare Kontext**.

Dazu gehören typischerweise:

- Hard IRQ
- SoftIRQ
- Tasklets
- Codebereiche mit bestimmten Spinlocks
- andere nicht schlafbare Kernelbereiche

Für atomaren Kontext gilt:

> Code darf nicht schlafen.

Deshalb ist die Frage:

```text
"Bin ich im Prozesskontext?"
```

nicht immer ausreichend.

Besser ist:

```text
"Darf dieser Code an dieser Stelle schlafen?"
```

---

## Merksatz

> **Im Linux-Kernel bestimmt der Ausführungskontext, welche Operationen erlaubt sind. Prozesskontext darf grundsätzlich schlafen, Interrupt- und atomarer Kontext dagegen nicht.**
