# Exkurs: seccomp

## Was ist seccomp?

**seccomp** steht für **Secure Computing Mode** und ist ein Sicherheitsmechanismus des Linux-Kernels.  
Damit kann ein Prozess einschränken, **welche System Calls er ausführen darf**.

Die Grundidee: Ein Prozess erhält nur die Kernel-Funktionen, die er tatsächlich benötigt. Dadurch wird die mögliche Angriffsfläche reduziert.

```text
Userspace-Prozess
      |
      | System Call
      v
+------------------+
| seccomp-Filter   |
+------------------+
      |
      +--> erlaubt  --> Kernel führt System Call aus
      |
      +--> verboten --> Fehler / Signal / Prozessende
```

## Warum ist seccomp sinnvoll?

Programme greifen über System Calls auf Kernel-Funktionen zu, zum Beispiel:

```text
read()   write()   openat()   socket()
execve() mount()   ptrace()   ioctl()
```

Wird eine Anwendung kompromittiert, könnte ein Angreifer grundsätzlich ebenfalls verfügbare System Calls nutzen.  
Mit seccomp lässt sich dieser Zugriff gezielt beschränken.

**Beispiel:** Eine Anwendung benötigt nur Datei-I/O, aber keine Netzwerk- oder Mount-Funktionen.  
Ein seccomp-Filter kann dann unter anderem `socket()` und `mount()` blockieren.

## seccomp-Modi

Linux unterscheidet im Wesentlichen folgende Modi:

| Wert in `/proc/<PID>/status` | Modus | Bedeutung |
|---|---|---|
| `0` | Disabled | seccomp ist nicht aktiv |
| `1` | Strict Mode | nur eine sehr kleine feste Menge von System Calls ist erlaubt |
| `2` | Filter Mode | System Calls werden durch einen Filter bewertet |

In der Praxis wird heute überwiegend **Filter Mode (`seccomp-BPF`)** verwendet.

Status eines Prozesses prüfen:

```bash
grep Seccomp /proc/<PID>/status
```

Beispiel:

```text
Seccomp:        2
```

## seccomp-BPF

Im Filtermodus wird für jeden System Call geprüft, ob er erlaubt ist.

Ein Filter kann beispielsweise festlegen:

```text
read      -> erlauben
write     -> erlauben
futex     -> erlauben
exit      -> erlauben
mount     -> blockieren
ptrace    -> blockieren
```

Je nach Regel kann ein nicht erlaubter System Call unterschiedliche Folgen haben, z. B.:

- Fehler zurückgeben
- `SIGSYS` auslösen
- Prozess beenden
- Ereignis protokollieren
- System Call für einen übergeordneten Prozess melden

## Einordnung im System-Call-Pfad

```text
Anwendung
    |
    v
System Call
    |
    v
seccomp / seccomp-BPF
    |
    +--> nicht erlaubt
    |
    v
Kernel System-Call-Handler
    |
    v
Kernel-Funktion
```

seccomp ist damit vereinfacht eine **Firewall für System Calls**.

## Typische Einsatzgebiete

seccomp wird häufig für Sandboxing und Isolation eingesetzt, unter anderem bei:

- Containern, z. B. Docker oder Kubernetes
- Browser-Sandboxen
- systemd-Diensten
- Server- und Netzwerkdiensten
- Anwendungen, die nach der Initialisierung nur noch wenige Kernel-Funktionen benötigen

## Abgrenzung

seccomp ersetzt keine klassischen Linux-Sicherheitsmechanismen.

```text
Dateirechte / UID / GID   -> Wer darf auf eine Ressource zugreifen?
Capabilities              -> Welche privilegierten Aktionen sind erlaubt?
SELinux / AppArmor        -> Auf welche Objekte darf ein Prozess zugreifen?
seccomp                   -> Welche System Calls darf ein Prozess ausführen?
```

## Merksatz

> **seccomp reduziert die Angriffsfläche eines Prozesses, indem es dessen Zugriff auf die System-Call-Schnittstelle des Linux-Kernels begrenzt.**
