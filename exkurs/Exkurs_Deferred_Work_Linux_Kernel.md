# Exkurs: Deferred Work im Linux-Kernel

## Warum gibt es Deferred Work?

Ein Hardware-Interrupt sollte im Linux-Kernel möglichst **kurz** behandelt werden.

Im unmittelbaren Interrupt-Kontext darf Kernel-Code unter anderem:

- nicht schlafen
- nicht auf I/O warten
- keine blockierenden Mutexes verwenden
- keine lang laufenden Arbeiten durchführen

Deshalb wird Arbeit häufig aufgeteilt:

```text
Hardware Interrupt
        |
        v
kurze Sofortarbeit
        |
        v
Deferred Work
        |
        +--> Threaded IRQ
        +--> SoftIRQ / NAPI
        +--> Workqueue
```

Die Idee lautet:

> **Zeitkritische Arbeit sofort erledigen, aufwendige Arbeit später ausführen.**

---

## 1. Threaded IRQ

Ein **Threaded IRQ** teilt die Interrupt-Behandlung in zwei Teile:

```text
Hardware IRQ
     |
     v
Primary Handler
     |
     | sehr kurz
     v
IRQ Thread
     |
     v
aufwendigere Verarbeitung
```

Der Primary Handler läuft im **Hard-IRQ-Kontext**.

Typische Aufgaben:

- Interruptquelle prüfen
- Gerät quittieren
- Interrupt ggf. maskieren
- Thread wecken

Der eigentliche Thread läuft dagegen im **Prozesskontext**.

Dadurch darf er grundsätzlich:

- schlafen
- Mutexes verwenden
- auf I/O warten
- aufwendigere Verarbeitung durchführen

Typische Registrierung:

```c
request_threaded_irq()
```

Threaded IRQs eignen sich gut für Treiber, bei denen die Interrupt-Nachbearbeitung etwas aufwendiger ist oder schlafen können muss.

---

## 2. SoftIRQ

**SoftIRQs** sind ein Kernel-Mechanismus für verzögerte Verarbeitung mit sehr geringer Latenz.

Sie werden häufig für stark frequentierte Kernel-Pfade verwendet.

Typische Beispiele:

- Netzwerk RX
- Netzwerk TX
- Timer
- RCU

Ablauf:

```text
Hardware IRQ
     |
     v
Hard IRQ
     |
     | SoftIRQ markieren
     v
SoftIRQ
```

SoftIRQs laufen weiterhin in einem **atomaren Kontext**.

Daher gilt:

```text
Schlafen:        nein
Blockieren:      nein
Mutex verwenden: nein
```

SoftIRQs sind dafür sehr effizient und eignen sich für hohe Ereignisraten.

---

## 3. NAPI

**NAPI – New API** ist ein Mechanismus des Linux-Netzwerkstacks.

NAPI kombiniert:

```text
Interrupts + Polling
```

Das Problem bei schnellen Netzwerkkarten:

```text
Paket
  -> Interrupt

Paket
  -> Interrupt

Paket
  -> Interrupt

...
```

Bei sehr hoher Netzwerklast kann die CPU sonst fast nur noch Interrupts bearbeiten.

NAPI arbeitet deshalb vereinfacht so:

```text
Netzwerkpaket
      |
      v
Hardware IRQ
      |
      v
Interrupts für RX reduzieren/deaktivieren
      |
      v
NAPI Poll
      |
      +--> mehrere Pakete verarbeiten
      +--> bis Budget erreicht
      |
      v
Interrupts wieder aktivieren
```

Statt für jedes Paket einen Interrupt auszulösen, verarbeitet Linux mehrere Pakete gebündelt.

NAPI wird typischerweise über den Netzwerk-SoftIRQ ausgeführt.

Damit gilt auch hier:

- kein Schlafen
- kein blockierender Mutex
- möglichst effizient arbeiten

### Warum ist NAPI sinnvoll?

Bei geringer Last:

```text
Interrupt
```

liefert niedrige Latenz.

Bei hoher Last:

```text
Polling
```

reduziert die Anzahl der Interrupts.

NAPI kombiniert beide Vorteile.

---

## 4. Workqueues

Eine **Workqueue** verschiebt Arbeit in einen Kernel-Worker-Thread.

```text
Hard IRQ
   |
   | Work einplanen
   v
Workqueue
   |
   v
kworker Thread
   |
   v
Work Function
```

Da die Work-Funktion von einem Kernel-Thread ausgeführt wird, läuft sie im **Prozesskontext**.

Damit sind möglich:

- schlafen
- Mutexes verwenden
- auf I/O warten
- längere Verarbeitung

Typische Verwendung:

```c
INIT_WORK()
schedule_work()
```

oder verzögert:

```c
INIT_DELAYED_WORK()
schedule_delayed_work()
```

Workqueues eignen sich besonders für Arbeiten, die:

- nicht zeitkritisch sind
- länger dauern können
- schlafen müssen
- nicht direkt im Interrupt-Kontext ausgeführt werden dürfen

---

## Vergleich

| Mechanismus | Kontext | Schlafen? | Typischer Einsatz |
|---|---|---:|---|
| Hard IRQ | Interruptkontext | Nein | unmittelbare Hardware-Reaktion |
| Threaded IRQ | Prozesskontext | Ja | Treiber-Nachbearbeitung |
| SoftIRQ | atomarer Kontext | Nein | Netzwerk, Timer, RCU |
| NAPI | SoftIRQ-Kontext | Nein | Netzwerk RX/TX |
| Workqueue | Prozesskontext | Ja | allgemeine verzögerte Arbeit |

---

## Typischer Treiberablauf

Ein Gerät erzeugt einen Interrupt:

```text
Hardware
   |
   v
Hard IRQ
   |
   +--> Gerät quittieren
   +--> Status lesen
   |
   +-----------------------------+
   |                             |
   v                             v
Threaded IRQ                  Workqueue
   |                             |
   v                             v
Treiberverarbeitung         längere Arbeit
```

Ein Netzwerktreiber verwendet dagegen häufig:

```text
NIC
 |
 v
Hardware IRQ
 |
 v
NAPI aktivieren
 |
 v
NET_RX SoftIRQ
 |
 v
napi_poll()
 |
 v
mehrere Pakete verarbeiten
```

---

## Auswahl des passenden Mechanismus

Vereinfacht:

```text
Muss es sofort passieren?
      |
      +--> Ja -> Hard IRQ

Muss die Nacharbeit sehr schnell erfolgen
und darf nicht schlafen?
      |
      +--> Ja -> SoftIRQ / NAPI

Ist es treiberspezifische IRQ-Nacharbeit,
die schlafen können soll?
      |
      +--> Threaded IRQ

Kann die Arbeit später erfolgen
und soll sie schlafen können?
      |
      +--> Workqueue
```

---

## Wichtig: Bottom Half

Historisch wurde Interrupt-Verarbeitung häufig in zwei Bereiche geteilt:

```text
Top Half
   -> unmittelbare Interrupt-Behandlung

Bottom Half
   -> verzögerte Verarbeitung
```

Heute werden verschiedene Mechanismen für diese verzögerte Arbeit verwendet, darunter:

- SoftIRQs
- NAPI
- Threaded IRQs
- Workqueues

Der Begriff **Deferred Work** ist daher allgemeiner als ein einzelner konkreter Kernel-Mechanismus.

---

## Merksatz

> **Deferred Work verschiebt aufwendige Arbeit aus dem unmittelbaren Interrupt-Kontext. SoftIRQ und NAPI sind besonders schnell, dürfen aber nicht schlafen; Threaded IRQs und Workqueues laufen dagegen im Prozesskontext und können blockierende Operationen verwenden.**
