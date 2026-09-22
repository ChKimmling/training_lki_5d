# Exkurs: Monolithischer Kernel vs. Microkernel

## Linux: monolithischer Kernel

Der Linux-Kernel ist ein **monolithischer Kernel mit ladbaren Modulen**.

Das bedeutet: Viele zentrale Betriebssystemfunktionen laufen gemeinsam im **Kernel Space** und können direkt miteinander kommunizieren.

Typische Bestandteile sind:

- Scheduler
- Speicherverwaltung
- Dateisysteme
- Netzwerkstack
- Gerätetreiber
- System-Call-Implementierungen
- Interrupt-Verarbeitung

```text
+-----------------------------+
|         Userspace           |
| Anwendungen                 |
| Shell, Dienste, Tools       |
+-----------------------------+
|        Kernel Space         |
| Scheduler                   |
| Speicherverwaltung          |
| Dateisysteme                |
| Netzwerkstack               |
| Gerätetreiber               |
| Kernelmodule                |
+-----------------------------+
|          Hardware           |
+-----------------------------+
```

## Gegenbeispiel: MINIX 3

**MINIX 3** verwendet einen **Microkernel-Ansatz**.

Beim Microkernel verbleiben nur wenige grundlegende Funktionen im Kernel. Viele weitere Betriebssystemdienste laufen als eigenständige Prozesse im Userspace.

```text
+-----------------------------+
|         Userspace           |
| Anwendungen                 |
| Dateisystem-Server          |
| Netzwerk-Server             |
| Gerätetreiber               |
| weitere Systemdienste       |
+-----------------------------+
|         Microkernel         |
| Scheduling                  |
| IPC                         |
| Interrupt-Verarbeitung      |
| grundlegende Speicherverw.  |
+-----------------------------+
|          Hardware           |
+-----------------------------+
```

## Grundidee des Microkernels

Der Kernel soll möglichst klein bleiben.

Subsysteme kommunizieren deshalb häufig über:

```text
IPC = Inter Process Communication
```

Beispiel:

```text
Anwendung
    |
    | Anfrage
    v
Dateisystem-Server
    |
    | IPC
    v
Microkernel
    |
    v
Gerätetreiber
```

Anstatt dass alle Komponenten direkt als Kernel-Funktionen miteinander arbeiten, sind viele Dienste voneinander getrennt.

## Vergleich

| Linux | MINIX 3 |
|---|---|
| monolithischer Kernel | Microkernel |
| viele Dienste im Kernel Space | viele Dienste im Userspace |
| Treiber meist im Kernel | viele Treiber als Userspace-Prozesse |
| direkte Kernel-Funktionsaufrufe | stärkere Nutzung von IPC |
| eng integrierte Subsysteme | stärkere Trennung der Komponenten |
| Fehler im Kernel-Code können das Gesamtsystem betreffen | Fehler einzelner Dienste können besser isoliert werden |

## Warum verwendet Linux einen monolithischen Kernel?

Ein monolithischer Aufbau ermöglicht eine sehr direkte Kommunikation zwischen Kernel-Subsystemen.

Beispielsweise:

```text
Netzwerktreiber
      |
      v
Netzwerkstack
      |
      v
Socket
```

Die beteiligten Komponenten befinden sich im selben Adressraum und können über direkte Funktionsaufrufe zusammenarbeiten.

Das reduziert den Kommunikationsaufwand zwischen den Subsystemen.

## Ladbare Kernelmodule

Linux ist zwar monolithisch aufgebaut, aber dennoch modular.

Viele Komponenten können als **Loadable Kernel Modules (LKM)** zur Laufzeit geladen oder entfernt werden.

Beispiele:

```bash
lsmod
modprobe <modul>
rmmod <modul>
```

Typische Module sind:

- Gerätetreiber
- Dateisysteme
- Netzwerkprotokolle
- zusätzliche Kernel-Funktionen

```text
Linux Kernel
     |
     +-- Core Kernel
     |
     +-- Modul: Treiber
     +-- Modul: Dateisystem
     +-- Modul: Netzwerkfunktion
```

Wichtig:

> Ein Kernelmodul läuft weiterhin im Kernel Space.

Linux wird dadurch also nicht zu einem Microkernel.

## Weitere Microkernel-Systeme

Neben MINIX 3 gibt es weitere bekannte Systeme mit Microkernel-Architektur.

### QNX Neutrino

QNX ist besonders im Embedded-, Automotive- und Echtzeitbereich verbreitet.

Typische Eigenschaften:

- Microkernel
- viele Dienste im Userspace
- Message Passing
- Echtzeitfähigkeit
- starke Isolation einzelner Komponenten

### seL4

**seL4** ist ein besonders kleiner Microkernel mit formal verifizierten Kerneleigenschaften und wird vor allem in sicherheitskritischen Systemen eingesetzt.

## Hybridkernel

Zwischen monolithischen Kerneln und Microkerneln existieren auch **Hybridkernel**.

Sie übernehmen Ideen aus beiden Ansätzen.

Ein bekanntes Beispiel ist die Windows-NT-Kernelarchitektur.

```text
Monolithisch       Hybrid          Microkernel

   Linux         Windows NT         MINIX 3
                                      QNX
                                      seL4
```

Die Grenzen zwischen den Kategorien sind in der Praxis nicht immer vollständig eindeutig.

## Kerngedanke

```text
Monolithischer Kernel:
Viele Betriebssystemdienste laufen im Kernel Space.

Microkernel:
Nur grundlegende Mechanismen laufen im Kernel.
Viele Dienste werden in den Userspace ausgelagert.
```

## Merksatz

> **Linux verfolgt einen monolithischen, aber modularen Kernelansatz. Microkernel wie MINIX 3 oder QNX halten den Kernel dagegen möglichst klein und verlagern viele Betriebssystemdienste in getrennte Userspace-Prozesse.**
