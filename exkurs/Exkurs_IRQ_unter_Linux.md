# Exkurs: IRQ unter Linux

## Was ist ein IRQ?

**IRQ** steht für **Interrupt Request**.  
Ein IRQ ist eine Anforderung eines Hardware-Geräts an die CPU, ihre aktuelle Arbeit kurz zu unterbrechen und ein bestimmtes Ereignis zu behandeln.

Typische Auslöser sind zum Beispiel:

- Tastatur- oder Mausereignisse
- Netzwerkpakete
- Timer
- Speicher- oder Controller-Ereignisse
- ACPI-Ereignisse
- Ein-/Ausgabe von Geräten

```text
Hardware-Gerät
     |
     | Interrupt Request
     v
Interrupt Controller
     |
     v
CPU
     |
     v
Linux IRQ Handler
```

## Warum braucht Linux Interrupts?

Ohne Interrupts müsste die CPU ständig prüfen, ob ein Gerät neue Daten bereitstellt. Das nennt man **Polling**.

Mit Interrupts kann die CPU andere Aufgaben bearbeiten und wird nur dann benachrichtigt, wenn ein Ereignis eintritt.

```text
Polling:
CPU -> Gerät prüfen -> Gerät prüfen -> Gerät prüfen -> ...

Interrupt:
CPU arbeitet weiter
       |
       +---- Gerät meldet Ereignis
                    |
                    v
                Interrupt
```

## IRQs anzeigen

Linux stellt Informationen über Interrupts über `/proc/interrupts` bereit:

```bash
cat /proc/interrupts
```

Beispiel:

```text
           CPU0       CPU1
  0:         47          0  IR-IO-APIC   2-edge      timer
  9:          0      31086  IR-IO-APIC   9-fasteoi   acpi
 24:       1523       1874  PCI-MSI      eth0
```

Die Spalten bedeuten:

| Feld | Bedeutung |
|---|---|
| `0:`, `9:`, `24:` | IRQ-Nummer |
| `CPU0`, `CPU1`, ... | Anzahl der Interrupts pro CPU |
| `IR-IO-APIC`, `PCI-MSI` | Interrupt-Controller bzw. Interrupt-Typ |
| `edge`, `fasteoi` | Art der Interrupt-Behandlung |
| `timer`, `acpi`, `eth0` | Gerät oder Kernel-Handler |

## Ablauf eines Interrupts

Vereinfacht läuft ein Hardware-Interrupt so ab:

```text
1. Gerät erzeugt Interrupt
        |
        v
2. Interrupt Controller erkennt IRQ
        |
        v
3. CPU unterbricht aktuellen Code
        |
        v
4. Kernel übernimmt Kontrolle
        |
        v
5. IRQ-Handler wird ausgeführt
        |
        v
6. Ereignis wird bestätigt
        |
        v
7. CPU setzt vorherige Arbeit fort
```

Der aktuelle Prozess wird dabei nicht beendet. Seine Ausführung wird lediglich kurz unterbrochen.

## Interrupt Context

Ein IRQ-Handler läuft in einem speziellen **Interrupt Context**.

Dabei gelten wichtige Einschränkungen:

- kein normaler Userspace-Prozesskontext
- Handler sollten möglichst kurz bleiben
- blockierende Operationen sind im Hard-IRQ-Kontext nicht erlaubt
- aufwendige Verarbeitung wird häufig verschoben

Daher wird Interrupt-Verarbeitung oft aufgeteilt:

```text
Interrupt
   |
   v
Hard IRQ
   |
   | kurze, zeitkritische Arbeit
   v
Deferred Work
   |
   +--> SoftIRQ
   +--> Tasklet
   +--> Workqueue
   +--> Threaded IRQ
```

## Shared IRQs

Mehrere Geräte können sich eine IRQ-Leitung teilen.

```text
Device A ----+
             |
Device B ----+---- IRQ 17 ----> CPU
             |
Device C ----+
```

Der Kernel ruft dann die registrierten Handler auf. Jeder Handler muss prüfen, ob das Ereignis tatsächlich von seinem Gerät stammt.

## Moderne Interrupts: MSI / MSI-X

Moderne PCI- und PCIe-Geräte verwenden häufig **MSI** oder **MSI-X** statt klassischer IRQ-Leitungen.

**MSI = Message Signaled Interrupt**

Dabei signalisiert das Gerät einen Interrupt durch eine spezielle Speichertransaktion.

Vorteile:

- keine physische Interrupt-Leitung notwendig
- weniger IRQ-Sharing
- mehrere Interrupt-Vektoren pro Gerät möglich
- bessere Verteilung auf mehrere CPUs

MSI-X wird beispielsweise häufig von schnellen Netzwerk- und NVMe-Geräten verwendet.

## IRQ-Verteilung auf CPUs

Auf Mehrkernsystemen können Interrupts auf bestimmte CPUs verteilt werden.

Anzeige:

```bash
cat /proc/interrupts
```

IRQ-Affinität:

```bash
cat /proc/irq/<IRQ>/smp_affinity
```

Linux kann Interrupts dadurch gezielt auf verschiedene CPU-Kerne verteilen. Der Dienst `irqbalance` übernimmt diese Verteilung auf vielen Systemen automatisch.

## Wichtige Begriffe

```text
IRQ                Interrupt Request
ISR                Interrupt Service Routine
APIC               Advanced Programmable Interrupt Controller
I/O APIC           Interrupt Controller für externe Geräte
MSI / MSI-X        Message Signaled Interrupts
Hard IRQ           unmittelbare Interrupt-Behandlung
SoftIRQ            verzögerte Kernel-Verarbeitung
IRQ Affinity       Zuordnung eines IRQs zu CPUs
```

## Merksatz

> **Ein IRQ ermöglicht es Hardware, die CPU asynchron über ein Ereignis zu informieren. Linux unterbricht daraufhin kurz die aktuelle Ausführung, behandelt das Ereignis im Kernel und setzt anschließend die vorherige Arbeit fort.**
