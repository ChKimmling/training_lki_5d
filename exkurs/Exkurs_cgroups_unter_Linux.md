# Exkurs: cgroups unter Linux

## Was sind cgroups?

**cgroups** steht für **Control Groups**.  
Sie sind ein Mechanismus des Linux-Kernels, mit dem Prozesse zu Gruppen zusammengefasst und deren **Ressourcenverbrauch begrenzt, priorisiert und überwacht** werden kann.

Typische Ressourcen sind:

- CPU-Zeit
- Arbeitsspeicher
- Anzahl von Prozessen
- Block-I/O
- Gerätezugriffe
- weitere kernelverwaltete Ressourcen

```text
Linux-Prozesse
     |
     +--> cgroup A
     |      +--> Prozess 1
     |      +--> Prozess 2
     |
     +--> cgroup B
            +--> Prozess 3
```

## Warum braucht Linux cgroups?

Ohne cgroups konkurrieren Prozesse grundsätzlich gemeinsam um verfügbare Systemressourcen.

Mit cgroups kann Linux beispielsweise festlegen:

```text
Webserver
   -> maximal 2 CPUs
   -> maximal 1 GiB RAM

Datenbank
   -> maximal 4 GiB RAM

Build-Prozess
   -> niedrige CPU-Priorität
```

Dadurch lassen sich Prozesse kontrollieren und voneinander hinsichtlich ihrer Ressourcennutzung abgrenzen.

## cgroup v1 und cgroup v2

Linux kennt zwei Generationen:

- **cgroup v1** – ältere Implementierung mit getrennten Hierarchien je Controller
- **cgroup v2** – moderne, vereinheitlichte Hierarchie

Aktuelle Linux-Systeme verwenden überwiegend **cgroup v2**.

Prüfen:

```bash
mount | grep cgroup
```

Typisch für cgroup v2:

```text
cgroup2 on /sys/fs/cgroup type cgroup2 (...)
```

## Die cgroup-Hierarchie

cgroup v2 verwendet eine Baumstruktur:

```text
/sys/fs/cgroup
       |
       +-- system.slice
       |      |
       |      +-- ssh.service
       |      +-- nginx.service
       |
       +-- user.slice
              |
              +-- user-1000.slice
```

Jeder Prozess befindet sich in einer cgroup.

Die Zuordnung eines Prozesses lässt sich anzeigen mit:

```bash
cat /proc/<PID>/cgroup
```

Bei cgroup v2 beispielsweise:

```text
0::/user.slice/user-1000.slice/session-2.scope
```

## Wichtige Controller

Controller bestimmen, welche Ressource eine cgroup verwalten kann.

| Controller | Aufgabe |
|---|---|
| `cpu` | CPU-Zeit und CPU-Gewichtung |
| `memory` | Speicherverbrauch |
| `io` | Block-I/O |
| `pids` | Anzahl der Prozesse |
| `cpuset` | erlaubte CPUs und NUMA-Nodes |

Verfügbare Controller:

```bash
cat /sys/fs/cgroup/cgroup.controllers
```

Beispiel:

```text
cpuset cpu io memory hugetlb pids
```

## Beispiel: Speicher begrenzen

Eine neue cgroup anlegen:

```bash
sudo mkdir /sys/fs/cgroup/demo
```

Speicherlimit auf 256 MiB setzen:

```bash
echo $((256 * 1024 * 1024)) | sudo tee /sys/fs/cgroup/demo/memory.max
```

Einen Prozess der Gruppe zuordnen:

```bash
echo <PID> | sudo tee /sys/fs/cgroup/demo/cgroup.procs
```

Der Prozess darf dann zusammen mit den anderen Prozessen dieser cgroup maximal den festgelegten Speicher verwenden.

## Beispiel: CPU steuern

Mit `cpu.max` kann CPU-Zeit begrenzt werden.

Beispiel:

```bash
echo "50000 100000" | sudo tee /sys/fs/cgroup/demo/cpu.max
```

Das bedeutet vereinfacht:

```text
50.000 µs CPU-Zeit
pro
100.000 µs Zeitraum

=> maximal ca. 50 % einer CPU
```

Mit `cpu.weight` lässt sich dagegen die relative CPU-Gewichtung zwischen Gruppen beeinflussen.

## cgroups und systemd

Auf modernen Distributionen verwaltet **systemd** die cgroup-Hierarchie.

Typische Gruppen:

```text
system.slice
user.slice
machine.slice
```

Ein Dienst:

```text
nginx.service
```

befindet sich beispielsweise typischerweise unter:

```text
/system.slice/nginx.service
```

Informationen lassen sich anzeigen mit:

```bash
systemd-cgls
```

oder:

```bash
systemd-cgtop
```

## cgroups und Container

Container verwenden cgroups, um ihren Ressourcenverbrauch zu begrenzen.

```text
Container A
   |
   +-- CPU-Limit
   +-- Memory-Limit
   +-- PID-Limit

Container B
   |
   +-- andere Limits
```

Docker-Optionen wie:

```bash
docker run --memory=512m --cpus=1 ...
```

werden unter Linux letztlich über cgroups umgesetzt.

## Namespaces vs. cgroups

Namespaces und cgroups erfüllen unterschiedliche Aufgaben:

```text
Namespaces
    -> Was kann ein Prozess sehen?

cgroups
    -> Welche Ressourcen kann ein Prozess nutzen?
```

Beispiel eines Containers:

```text
Container
   |
   +-- PID Namespace
   +-- Network Namespace
   +-- Mount Namespace
   |
   +-- cgroup
          +-- CPU-Limit
          +-- Memory-Limit
          +-- PID-Limit
```

**Namespaces sorgen für Isolation der Systemsicht, cgroups für Ressourcensteuerung.**

## cgroup Namespace

Zusätzlich existiert ein eigener **cgroup Namespace**.

Er isoliert nicht die Ressourcenlimits selbst, sondern die **Sicht eines Prozesses auf die cgroup-Hierarchie**.

Damit kann ein Prozess innerhalb eines Containers seine eigene cgroup als Wurzel sehen, obwohl sie auf dem Host tiefer in der Hierarchie liegt.

```text
Host:
 /system.slice/container.scope

Container:
 /
```

Der cgroup Namespace ergänzt damit die Isolation, die Container bereits durch andere Namespaces erhalten.

## Merksatz

> **cgroups kontrollieren, wie viele Ressourcen Prozesse verwenden dürfen. Namespaces kontrollieren, welche Systemressourcen und Systemobjekte Prozesse sehen können. Zusammen bilden sie eine zentrale Grundlage moderner Linux-Container.**
