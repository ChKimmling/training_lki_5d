# Exkurs: Linux Namespaces bei Prozessen

## Was sind Namespaces?

**Namespaces** sind ein Mechanismus des Linux-Kernels zur **Isolation von Prozessen**.

Ein Prozess sieht normalerweise viele globale Systemressourcen, zum Beispiel:

- andere Prozesse
- Netzwerk-Interfaces
- Mountpoints
- Hostname
- Benutzer- und Gruppen-IDs
- IPC-Ressourcen

Namespaces sorgen dafür, dass ein Prozess nur eine **isolierte Sicht** auf bestimmte Ressourcen bekommt.

```text
Ohne Namespace:
Prozess A ----+
Prozess B ----+---- gemeinsame Systemsicht
Prozess C ----+

Mit Namespaces:
Prozess A ---> eigene Sicht
Prozess B ---> eigene Sicht
Prozess C ---> andere Sicht
```

Namespaces sind eine zentrale Grundlage von **Containern**.

## Wichtige Namespace-Typen

Linux kennt mehrere Namespace-Arten:

| Namespace | Isolation von |
|---|---|
| `pid` | Prozess-IDs und Prozesshierarchie |
| `net` | Netzwerk-Interfaces, Routing, Ports |
| `mnt` | Mountpoints und Dateisystem-Sicht |
| `uts` | Hostname und Domainname |
| `ipc` | Shared Memory, Semaphore, Message Queues |
| `user` | User- und Group-IDs |
| `cgroup` | Sicht auf die Cgroup-Hierarchie |
| `time` | bestimmte Systemzeit-Offsets |

## Beispiel: PID Namespace

Ein Prozess kann innerhalb eines PID-Namespaces eine andere PID besitzen als außerhalb.

```text
Host:
PID 4321
   |
   +---- Container / PID Namespace
            |
            +---- PID 1
```

Der Prozess ist also aus Sicht des Hosts beispielsweise **PID 4321**, sieht sich innerhalb seines Namespace aber als **PID 1**.

Dadurch erhält jeder Container eine eigene Prozesshierarchie.

## Beispiel: Network Namespace

Ein Network Namespace besitzt eine eigene Netzwerksicht:

```text
Network Namespace A
- eth0
- eigene IP-Adresse
- eigene Routing-Tabelle
- eigene Firewall-Regeln

Network Namespace B
- eth0
- andere IP-Adresse
- andere Routing-Tabelle
```

Zwei Prozesse können dadurch sogar denselben TCP-Port verwenden, solange sie sich in unterschiedlichen Network Namespaces befinden.

## Namespaces eines Prozesses anzeigen

Die Namespaces eines Prozesses sind unter `/proc` sichtbar:

```bash
ls -l /proc/<PID>/ns
```

Beispiel:

```text
cgroup -> cgroup:[4026531835]
ipc    -> ipc:[4026531839]
mnt    -> mnt:[4026531841]
net    -> net:[4026531992]
pid    -> pid:[4026531836]
user   -> user:[4026531837]
uts    -> uts:[4026531838]
```

Die Nummer in den eckigen Klammern identifiziert die jeweilige Namespace-Instanz.

Zwei Prozesse befinden sich im selben Namespace, wenn die IDs übereinstimmen.

## Namespaces mit `lsns` untersuchen

Eine praktische Übersicht liefert:

```bash
lsns
```

Oder nur Namespaces eines bestimmten Prozesses:

```bash
lsns -p <PID>
```

Typische Ausgabe:

```text
NS TYPE   NPROCS PID USER COMMAND
... mnt      42   1 root /sbin/init
... pid      42   1 root /sbin/init
... net      38   1 root /sbin/init
```

## Namespaces erzeugen

Mit `unshare` kann eine Shell in neuen Namespaces gestartet werden.

Beispiel:

```bash
sudo unshare --pid --fork --mount-proc bash
```

Die neue Shell läuft in einem eigenen PID Namespace.

Danach:

```bash
ps
```

Die Prozessliste zeigt nur noch die Prozesse dieses Namespace.

Weitere Beispiele:

```bash
unshare --net bash
unshare --mount bash
unshare --uts bash
```

## Prozesse in Namespaces starten

Im Kernel werden Namespaces unter anderem über folgende System Calls verwaltet:

```text
clone()
unshare()
setns()
```

- `clone()` kann einen neuen Prozess direkt in neuen Namespaces erzeugen.
- `unshare()` trennt einen Prozess von einem bestehenden Namespace.
- `setns()` lässt einen Prozess einem vorhandenen Namespace beitreten.

## Namespaces und Container

Container verwenden typischerweise mehrere Namespaces gleichzeitig:

```text
Container
   |
   +-- PID Namespace
   +-- Network Namespace
   +-- Mount Namespace
   +-- UTS Namespace
   +-- IPC Namespace
   +-- User Namespace
```

Dadurch sieht ein Prozess im Container eine scheinbar eigene Linux-Umgebung.

Wichtig:

> Ein Container besitzt normalerweise **keinen eigenen Kernel**.

Alle Container verwenden weiterhin denselben Linux-Kernel des Hosts.

```text
Container A     Container B
     |               |
     +-------+-------+
             |
        Linux Kernel
             |
          Hardware
```

## Namespaces vs. Cgroups

Namespaces und Cgroups erfüllen unterschiedliche Aufgaben:

```text
Namespaces
    -> Was darf ein Prozess sehen?

Cgroups
    -> Wie viele Ressourcen darf ein Prozess verwenden?
```

Beispiele für Cgroup-Ressourcen:

- CPU-Zeit
- Arbeitsspeicher
- I/O-Bandbreite
- Anzahl von Prozessen

Container kombinieren deshalb meist **Namespaces + Cgroups**.

## Merksatz

> **Linux Namespaces isolieren die Sicht eines Prozesses auf Systemressourcen. Sie bilden zusammen mit Cgroups eine zentrale technische Grundlage für Container.**
