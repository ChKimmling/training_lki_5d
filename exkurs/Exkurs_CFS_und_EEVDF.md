# Exkurs: CFS und EEVDF im Linux-Scheduler

## Einordnung

Für normale Userspace-Prozesse verwendet Linux die **Fair-Scheduling-Klasse**:

```text
SCHED_NORMAL / SCHED_OTHER
SCHED_BATCH
SCHED_IDLE
```

Historisch wurde diese Klasse vor allem durch den **CFS – Completely Fair Scheduler** geprägt.

Seit Linux 6.6 verwendet der Fair Scheduler für die Auswahl des nächsten Tasks zunehmend das Verfahren:

```text
EEVDF
= Earliest Eligible Virtual Deadline First
```

Wichtig:

> **EEVDF ersetzt nicht die gesamte Fair-Scheduler-Infrastruktur, sondern die zentrale Auswahlstrategie innerhalb der Fair-Scheduling-Klasse.**

---

## CFS – Completely Fair Scheduler

Die Grundidee von CFS lautet:

> Jeder laufbereite Task soll entsprechend seiner Gewichtung einen möglichst fairen Anteil an CPU-Zeit erhalten.

CFS verwendet dafür die sogenannte:

```text
vruntime
```

**vruntime = virtual runtime**

Sie beschreibt vereinfacht, wie viel CPU-Zeit ein Task bereits erhalten hat – gewichtet nach seiner Priorität.

---

## `vruntime`

Ein Task mit kleiner `vruntime` wurde relativ gesehen weniger stark bedient.

Vereinfacht:

```text
Task A   vruntime = 10
Task B   vruntime = 14
Task C   vruntime = 21
```

CFS bevorzugt den Task mit der kleinsten `vruntime`:

```text
Task A
```

Historisch wurden laufbereite Tasks dafür in einem Red-Black Tree verwaltet.

```text
           vruntime
              |
        +-----+-----+
        |           |
     Task A       Task C
        |
     Task B
```

Das linkeste Element entspricht dabei dem Task mit der kleinsten `vruntime`.

---

## Einfluss von Nice-Werten

Die CPU-Zeit wird gewichtet.

Beispiel:

```text
nice -5   -> höheres Gewicht
nice  0   -> normales Gewicht
nice +10  -> geringeres Gewicht
```

Ein höher gewichteter Task sammelt `vruntime` langsamer an und erhält dadurch relativ mehr CPU-Zeit.

Vereinfacht:

```text
reale Laufzeit
     |
     v
Gewichtung durch nice
     |
     v
vruntime
```

---

## Grenzen des klassischen CFS-Ansatzes

CFS ist sehr gut bei langfristiger Fairness.

Bei der Auswahl des nächsten Tasks betrachtet ein rein vruntime-basierter Ansatz aber vor allem:

```text
Wer wurde bisher am wenigsten bedient?
```

Für gute Reaktionszeiten ist zusätzlich interessant:

```text
Wer sollte als Nächstes dran sein?
```

Hier setzt EEVDF an.

---

# EEVDF – Earliest Eligible Virtual Deadline First

EEVDF kombiniert Fairness mit einer virtuellen Deadline.

Zwei zentrale Konzepte sind:

```text
Eligibility
Virtual Deadline
```

Die Auswahl läuft vereinfacht so:

```text
1. Welche Tasks sind eligible?
2. Welcher davon hat die früheste virtuelle Deadline?
```

---

## Eligibility

Ein Task ist **eligible**, wenn er im Verhältnis zu seinem fairen Anteil nicht zu weit voraus ist.

Dazu betrachtet der Scheduler den sogenannten:

```text
lag
```

Vereinfacht:

```text
lag > 0
-> Task hat relativ betrachtet noch CPU-Zeit "gut"

lag < 0
-> Task wurde bereits stärker bedient
```

Nur Tasks, die als ausreichend fair gelten, nehmen unmittelbar an der Deadline-Auswahl teil.

---

## Virtual Deadline

Für einen Task wird eine virtuelle Deadline bestimmt.

Vereinfacht:

```text
virtual deadline
=
vruntime + virtuelle Zeitscheibe
```

Der Scheduler bevorzugt anschließend unter den **eligible Tasks** den mit der frühesten Deadline.

Beispiel:

```text
Task A: eligible, deadline 40
Task B: eligible, deadline 32
Task C: nicht eligible
```

Auswahl:

```text
Task B
```

---

## Warum EEVDF?

EEVDF soll zwei Ziele besser miteinander verbinden:

```text
Fairness
   +
geringe Latenz
```

Besonders kurze bzw. interaktive Aufgaben können dadurch früher berücksichtigt werden, ohne die langfristige Fairness aufzugeben.

Vereinfacht:

```text
CFS:
"Wer hat am wenigsten virtuelle Laufzeit?"

EEVDF:
"Wer ist fairerweise dran und hat die früheste virtuelle Deadline?"
```

---

## Requested Slice

Bei EEVDF spielt auch die gewünschte Zeitscheibe eines Tasks eine Rolle.

Ein Task kann sinngemäß eine bestimmte Laufzeit anfordern:

```text
requested slice
```

Diese fließt in die virtuelle Deadline ein.

Kürzere gewünschte Laufzeiten können damit zu früheren Deadlines führen.

Das ist insbesondere für latenzempfindliche Tasks interessant.

---

## Vergleich

| CFS | EEVDF |
|---|---|
| Fokus auf `vruntime` | `vruntime` + Eligibility + Deadline |
| kleinste `vruntime` bevorzugen | früheste Deadline unter eligible Tasks |
| langfristige Fairness | Fairness + bessere Latenzsteuerung |
| historischer Kern des Fair Schedulers | moderne Auswahlstrategie |
| Nice beeinflusst Gewicht | Nice beeinflusst weiterhin Gewicht |

---

## Was bleibt gleich?

Auch mit EEVDF bleiben viele bekannte Mechanismen erhalten:

```text
nice
scheduler weights
vruntime
SCHED_NORMAL
CFS bandwidth control
cgroups
load balancing
```

EEVDF ist also keine komplett neue Scheduling-Klasse.

Es ist eine Weiterentwicklung der Auswahlstrategie innerhalb des bestehenden Fair Schedulers.

---

## Vereinfachtes Gesamtbild

```text
Runnable Tasks
     |
     v
Fair Scheduler
     |
     +--> Gewicht / nice
     +--> vruntime
     +--> lag
     +--> eligibility
     +--> virtual deadline
     |
     v
früheste Deadline
unter eligible Tasks
     |
     v
nächster Task
```

---

## Beispiel

Angenommen:

```text
Task A
vruntime = 100
slice    = 20
deadline = 120

Task B
vruntime = 105
slice    = 5
deadline = 110
```

Wenn beide eligible sind:

```text
Task B
```

wird bevorzugt, weil seine virtuelle Deadline früher liegt.

Damit kann ein kurzer Task schneller CPU-Zeit erhalten, obwohl seine `vruntime` nicht die kleinste ist.

---

## Nicht mit Echtzeit-Scheduling verwechseln

EEVDF verwendet zwar den Begriff:

```text
deadline
```

ist aber **kein Hard-Realtime-Scheduler**.

Nicht verwechseln mit:

```text
SCHED_FIFO
SCHED_RR
SCHED_DEADLINE
```

Diese gehören zu anderen Scheduling-Klassen bzw. Echtzeitmechanismen.

EEVDF arbeitet weiterhin innerhalb des normalen Fair Schedulers.

---

## Merksatz

> **CFS verteilt CPU-Zeit anhand der virtuellen Laufzeit möglichst fair. EEVDF erweitert diese Idee, indem der Scheduler zunächst faire („eligible“) Tasks bestimmt und anschließend den Task mit der frühesten virtuellen Deadline auswählt – mit dem Ziel, Fairness und Reaktionszeit besser zu verbinden.**
