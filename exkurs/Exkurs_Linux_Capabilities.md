# Exkurs: Linux Capabilities

## Definition

**Linux Capabilities** zerlegen die klassischen Root-Rechte in einzelne, gezielt vergebbare Berechtigungen. Ein Prozess erhält dadurch nur die Privilegien, die er tatsächlich benötigt, ohne mit vollständigen Root-Rechten ausgeführt werden zu müssen.

## Typische Capabilities

| Capability | Erlaubt unter anderem |
|---|---|
| `CAP_NET_ADMIN` | Netzwerkschnittstellen, Routing und Firewall-Regeln konfigurieren |
| `CAP_NET_BIND_SERVICE` | An privilegierte Ports unter 1024 binden |
| `CAP_SYS_ADMIN` | Zahlreiche systemnahe Verwaltungsoperationen; besonders weitreichend |
| `CAP_SYS_PTRACE` | Andere Prozesse untersuchen oder debuggen |
| `CAP_CHOWN` | Besitzer und Gruppe von Dateien ändern |
| `CAP_DAC_OVERRIDE` | Bestimmte klassische Dateizugriffsprüfungen umgehen |

## Prinzip der minimalen Rechte

Nach dem **Least-Privilege-Prinzip** erhält ein Programm nur die Berechtigungen, die es für seine Aufgabe benötigt. Ein Webserver, der Port 80 öffnen soll, braucht beispielsweise nicht alle Root-Rechte, sondern lediglich `CAP_NET_BIND_SERVICE`.

## Beispiel: Capability für eine Programmdatei

```bash
sudo setcap cap_net_bind_service=+ep /usr/bin/myserver
```

Die gesetzten Datei-Capabilities lassen sich anzeigen mit:

```bash
getcap /usr/bin/myserver
```

`+ep` trägt die Capability in die **Permitted**- und **Effective**-Menge der Datei ein.

## Capabilities eines laufenden Prozesses

Die Capability-Mengen eines Prozesses stehen in:

```bash
grep '^Cap' /proc/<PID>/status
```

Die Werte werden dort als Bitmasken in hexadezimaler Form ausgegeben. Zur leichteren Interpretation kann beispielsweise `capsh --decode=<WERT>` verwendet werden.

## Capability-Mengen

- **Permitted:** Obergrenze der Capabilities, die der Prozess verwenden oder in seine Effective-Menge übernehmen darf.
- **Effective:** Capabilities, die der Kernel bei Berechtigungsprüfungen aktuell berücksichtigt.
- **Inheritable:** Capabilities, die unter bestimmten Bedingungen beim Start eines neuen Programms weitergegeben werden können.
- **Bounding:** Systemseitige Obergrenze für Capabilities, die ein Prozess und seine Nachkommen durch einen Programmstart erhalten können.
- **Ambient:** Capabilities, die bei einem Programmstart auch ohne passende Datei-Capabilities erhalten bleiben können; sie müssen zugleich Permitted und Inheritable sein.

> **Merksatz:** Linux Capabilities teilen Root-Rechte in einzelne Privilegien auf und ermöglichen so, Prozesse nach dem Prinzip der minimalen Rechte auszuführen.
