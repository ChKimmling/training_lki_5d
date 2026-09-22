# Exkurs: Mutex, Semaphore, Spinlock, `raw_spinlock` und `local_lock_t`

## Warum braucht der Kernel Locks?

Im Linux-Kernel greifen oft mehrere Ausführungskontexte gleichzeitig auf gemeinsame Daten zu:

- mehrere Prozesse
- mehrere CPUs
- Interrupt-Handler
- SoftIRQs
- Kernel-Threads

Ohne Synchronisation entstehen **Race Conditions**.

Beispiel:

```text
CPU0                 CPU1
 |                    |
 | read counter       | read counter
 | counter++          | counter++
 | write              | write
```

Das Ergebnis kann falsch sein.

Locks verhindern solche konkurrierenden Zugriffe.

---

## 1. Mutex

Ein **Mutex** schützt kritische Abschnitte im **Prozesskontext**.

Typisch:

```c
struct mutex lock;

mutex_lock(&lock);
/* kritischer Bereich */
mutex_unlock(&lock);
```

Eigenschaften:

- nur ein Besitzer gleichzeitig
- darf schlafen
- blockierende Operationen sind möglich
- nicht für Hard-IRQ-Kontext geeignet

Wenn der Mutex belegt ist:

```text
Task A hält Mutex
       |
       v
Task B wartet
       |
       v
Scheduler kann Task B schlafen legen
```

### Typischer Einsatz

- Treiberlogik
- Konfigurationsdaten
- langsame kritische Abschnitte
- Bereiche mit möglichem Sleep

---

## 2. Semaphore

Eine **Semaphore** verwaltet einen Zähler.

Beispiel:

```c
struct semaphore sem;
```

Initialisierung:

```c
sema_init(&sem, 3);
```

Dann können bis zu drei Benutzer gleichzeitig eintreten.

```text
Zähler = 3

Task A -> erlaubt
Task B -> erlaubt
Task C -> erlaubt
Task D -> wartet
```

Typische Operationen:

```c
down(&sem);
up(&sem);
```

Eigenschaften:

- kann schlafen
- geeignet für Prozesskontext
- kann mehrere gleichzeitige Benutzer erlauben

### Binary Semaphore

Mit Zähler `1` ähnelt sie einem Mutex:

```text
Semaphore(1)
```

Im Kernel wird für gegenseitigen Ausschluss heute meist trotzdem ein echter Mutex bevorzugt.

---

## 3. Spinlock

Ein **Spinlock** wird verwendet, wenn der kritische Abschnitt kurz ist und nicht geschlafen werden darf.

Beispiel:

```c
spinlock_t lock;

spin_lock(&lock);
/* sehr kurzer kritischer Bereich */
spin_unlock(&lock);
```

Wenn der Lock belegt ist:

```text
CPU0 hält Lock
       |
       v
CPU1 wartet aktiv
       |
       v
"spinnt"
```

Das bedeutet:

> Die CPU bleibt aktiv und prüft wiederholt, ob der Lock frei wird.

Eigenschaften:

- kein Sleep
- für atomaren Kontext geeignet
- kurze kritische Abschnitte
- häufig bei SMP und Interrupt-Synchronisation

---

## Spinlock + Interrupts

Wenn Daten sowohl aus Prozesskontext als auch aus IRQ-Kontext verwendet werden, reicht ein einfacher Spinlock oft nicht.

Typisch:

```c
unsigned long flags;

spin_lock_irqsave(&lock, flags);
/* kritischer Bereich */
spin_unlock_irqrestore(&lock, flags);
```

Dadurch werden lokale Interrupts zusätzlich deaktiviert.

Vereinfacht:

```text
CPU
 |
 +--> Interrupts lokal aus
 |
 +--> Spinlock nehmen
 |
 +--> kritischer Bereich
 |
 +--> Spinlock freigeben
 |
 +--> Interruptzustand restaurieren
```

---

## 4. `raw_spinlock_t`

Ein **`raw_spinlock_t`** ist die niedrigere, besonders harte Form des Spinlocks.

Beispiel:

```c
raw_spinlock_t lock;

raw_spin_lock(&lock);
/* extrem kritischer Bereich */
raw_spin_unlock(&lock);
```

Er wird vor allem in sehr tiefen Kernelbereichen eingesetzt.

Wichtig bei **PREEMPT_RT**:

```text
spinlock_t
```

kann auf RT-Kernels in eine schlafbare Lock-Variante umgewandelt werden.

Ein:

```text
raw_spinlock_t
```

bleibt dagegen ein echter Busy-Wait-Spinlock.

Deshalb gilt:

> `raw_spinlock_t` nur verwenden, wenn wirklich harte Nicht-Schlaf-Semantik erforderlich ist.

Typische Bereiche:

- Scheduler
- Interrupt-Controller
- sehr low-level Kernelcode

---

## 5. `local_lock_t`

`local_lock_t` schützt Daten, die **CPU-lokal** sind.

Beispiel:

```c
local_lock_t lock;
```

Es geht also nicht primär darum, andere CPUs auszuschließen, sondern konkurrierende Zugriffe auf derselben CPU zu kontrollieren.

Typischer Zusammenhang:

```text
per-CPU data
```

Beispiel:

```c
DEFINE_PER_CPU(int, counter);
```

und Schutz über:

```c
local_lock(&lock);
/* Zugriff auf lokale CPU-Daten */
local_unlock(&lock);
```

---

## Warum `local_lock_t`?

Früher wurden per-CPU-Daten oft direkt durch Abschalten von Preemption oder Interrupts geschützt.

Zum Beispiel:

```c
preempt_disable();
...
preempt_enable();
```

`local_lock_t` macht diese Absicht expliziter.

Vorteile:

- bessere Dokumentation
- klarere Lock-Semantik
- bessere Unterstützung für PREEMPT_RT

---

## PREEMPT_RT und Lock-Verhalten

Bei einem Echtzeitkernel ändern sich einige Lock-Eigenschaften.

Vereinfacht:

```text
Normaler Kernel:
spinlock_t -> Busy-Wait-Spinlock

PREEMPT_RT:
spinlock_t -> weitgehend schlafbarer RT-Mutex

raw_spinlock_t -> bleibt echter Spinlock
```

Auch `local_lock_t` kann unter PREEMPT_RT anders implementiert werden, um Preemptibility besser zu erhalten.

Darum ist die Wahl des richtigen Lock-Typs besonders wichtig.

---

## Vergleich

| Mechanismus | Darf schlafen? | Typischer Kontext | Besonderheit |
|---|---:|---|---|
| `mutex` | Ja | Prozesskontext | klassischer Exklusiv-Lock |
| `semaphore` | Ja | Prozesskontext | Zähler > 1 möglich |
| `spinlock_t` | Nein* | atomarer Kontext / SMP | aktives Warten |
| `raw_spinlock_t` | Nein | sehr low-level | bleibt auch unter PREEMPT_RT raw |
| `local_lock_t` | abhängig von Kontext/RT | per-CPU-Daten | schützt lokale CPU-Ressourcen |

\* Auf PREEMPT_RT kann `spinlock_t` intern schlafbar umgesetzt werden.

---

## Wann welcher Lock?

Vereinfacht:

```text
Muss der Code schlafen können?
        |
        +--> Ja -> Mutex / Semaphore
        |
        +--> Nein
              |
              +--> mehrere CPUs?
              |      |
              |      +--> Spinlock
              |
              +--> nur lokale CPU-Daten?
                     |
                     +--> local_lock_t
```

Für besonders tiefe Kernelbereiche:

```text
raw_spinlock_t
```

---

## Typische Fehler

### Mutex im IRQ-Kontext

Falsch:

```c
irq_handler()
{
    mutex_lock(&lock);
}
```

Problem:

```text
Mutex kann schlafen
IRQ-Kontext darf nicht schlafen
```

### Zu langer Spinlock

Falsch:

```c
spin_lock(&lock);

msleep(100);

spin_unlock(&lock);
```

Problem:

```text
Spinlock hält CPU aktiv
msleep() darf schlafen
```

Das ist konzeptionell falsch.

---

## Merksatz

> **Mutex und Semaphore sind für schlafbaren Prozesskontext gedacht. Spinlocks schützen kurze atomare Bereiche. `raw_spinlock_t` bleibt auch unter PREEMPT_RT ein echter Spinlock, und `local_lock_t` schützt vor allem CPU-lokale Daten und macht lokale Synchronisation explizit.**
