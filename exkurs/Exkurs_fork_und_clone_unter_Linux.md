# Exkurs: `fork()` und `clone()` unter Linux

## Grundidee

Unter Linux entstehen neue Prozesse und Threads über Kernel-Mechanismen, die neue **Tasks** erzeugen.

Zwei wichtige Schnittstellen sind:

```text
fork()
clone()
```

Vereinfacht:

```text
fork()
  -> neuen Prozess erzeugen

clone()
  -> neuen Task mit gezielt teilbaren Ressourcen erzeugen
```

`clone()` ist dabei flexibler und bildet die Grundlage für viele Linux-spezifische Prozess-, Thread- und Namespace-Mechanismen.

## `fork()` – klassischen Kindprozess erzeugen

Ein Aufruf von:

```c
pid_t pid = fork();
```

erzeugt einen neuen Prozess.

Danach existieren:

```text
Parent
  |
  +--> Child
```

Beide Prozesse setzen die Ausführung **hinter `fork()`** fort.

Unterschied:

```text
Parent: fork() liefert PID des Child
Child:  fork() liefert 0
```

Beispiel:

```c
pid_t pid = fork();

if (pid == 0) {
    /* Child */
} else {
    /* Parent */
}
```

## Was wird bei `fork()` kopiert?

Nach `fork()` besitzt das Child eine weitgehend eigene Prozesssicht.

Typisch:

```text
eigene PID
eigener virtueller Adressraum
eigene Prozesshierarchie
eigene Signalzustände
```

Viele Ressourcen werden jedoch zunächst geteilt oder referenziert.

Besonders wichtig ist:

```text
Copy-on-Write
```

## Copy-on-Write bei `fork()`

Der komplette Speicher wird beim `fork()` nicht sofort kopiert.

Stattdessen zeigen Parent und Child zunächst auf dieselben physischen Speicherseiten:

```text
Parent ----+
           +--> gleiche physische Seite
Child  ----+
```

Die Seiten werden schreibgeschützt markiert.

Schreibt einer der Prozesse:

```text
Write
  |
  v
Page Fault
  |
  v
Seite kopieren
```

Danach:

```text
Parent --> Originalseite
Child  --> eigene Kopie
```

Das macht `fork()` deutlich effizienter.

## File Descriptors nach `fork()`

Geöffnete File Descriptors werden vererbt.

Beispiel:

```text
Parent fd 3 ----+
                +--> struct file
Child  fd 3 ----+
```

Parent und Child besitzen jeweils eigene FD-Tabellen, die Einträge können aber auf dasselbe `struct file` zeigen.

Dadurch können unter anderem gemeinsam genutzt werden:

```text
Dateiposition
Open-Flags
Dateiobjekt
```

## Typisches Muster: `fork()` + `exec()`

Sehr häufig wird ein neuer Prozess erzeugt und anschließend durch ein anderes Programm ersetzt.

```text
Parent
  |
  +--> fork()
         |
         +--> Child
                |
                +--> execve()
                       |
                       v
                 neues Programm
```

Beispiel:

```c
if (fork() == 0) {
    execl("/bin/ls", "ls", NULL);
}
```

Dieses Muster ist zentral für Shells.

## `clone()` – flexibler Task-Aufbau

`clone()` erlaubt gezielt festzulegen, welche Ressourcen Parent und Child teilen.

Vereinfacht:

```c
clone(fn, stack, flags, arg);
```

Entscheidend sind die:

```text
flags
```

Damit wird gesteuert, welche Teile gemeinsam genutzt werden.

## Typische `clone()`-Flags

| Flag | Bedeutung |
|---|---|
| `CLONE_VM` | virtuellen Adressraum teilen |
| `CLONE_FILES` | File-Descriptor-Tabelle teilen |
| `CLONE_FS` | Dateisystemkontext teilen |
| `CLONE_SIGHAND` | Signal-Handler teilen |
| `CLONE_THREAD` | gleicher Thread-Group angehören |
| `CLONE_NEWNS` | neuen Mount-Namespace erzeugen |
| `CLONE_NEWPID` | neuen PID-Namespace erzeugen |
| `CLONE_NEWNET` | neuen Network-Namespace erzeugen |
| `CLONE_NEWUSER` | neuen User-Namespace erzeugen |

Damit kann `clone()` sehr unterschiedliche Arten von Tasks erzeugen.

## Prozessähnlich vs. Threadähnlich

Ohne starke Ressourcenteilung:

```text
clone()
   |
   v
eher neuer Prozess
```

Mit Flags wie:

```text
CLONE_VM
CLONE_FILES
CLONE_SIGHAND
CLONE_THREAD
```

entsteht ein Task, der sich wie ein Thread verhält.

Vereinfacht:

```text
Prozess A
   |
   +--> Thread 1
   +--> Thread 2
   +--> Thread 3
```

Diese Threads teilen typischerweise:

```text
Adressraum
Dateideskriptoren
Signal-Handler
weitere Prozessressourcen
```

## Threads unter Linux

Linux unterscheidet intern nicht fundamental zwischen „Prozess“ und „Thread“.

Beides sind Kernel-Tasks.

Typisch:

```text
task_struct
```

Der Unterschied entsteht vor allem dadurch, **welche Ressourcen geteilt werden**.

Darum kann man vereinfacht sagen:

> Ein Thread ist unter Linux ein Task, der viele Ressourcen mit anderen Tasks teilt.

## `pthread_create()` und `clone()`

Anwendungen verwenden normalerweise nicht direkt `clone()`.

Stattdessen:

```c
pthread_create()
```

Die POSIX-Thread-Bibliothek nutzt intern Linux-spezifische Mechanismen zur Erzeugung eines neuen Tasks.

Vereinfacht:

```text
pthread_create()
      |
      v
libc / pthread
      |
      v
clone()/clone3()
      |
      v
Linux Kernel
```

## `clone3()`

Neben `clone()` gibt es heute auch:

```text
clone3()
```

`clone3()` verwendet eine strukturierte Argumentübergabe und ist flexibler erweiterbar.

Vereinfacht:

```c
struct clone_args args;
clone3(&args, sizeof(args));
```

Das erleichtert neue Kernel-Erweiterungen gegenüber der älteren `clone()`-Schnittstelle.

## `fork()` im Verhältnis zu `clone()`

Konzeptionell kann man `fork()` als eine speziellere Form der Task-Erzeugung betrachten.

```text
fork()
   |
   v
neuer Prozess
mit weitgehend getrennten Ressourcen

clone()
   |
   v
gezielt festlegen:
Was teilen?
Was trennen?
```

Im Kernel laufen die verschiedenen Erzeugungspfade letztlich über gemeinsame interne Mechanismen zur Task-Erzeugung.

## Namespaces mit `clone()`

`clone()` ist auch für Container-Techniken wichtig.

Beispiel:

```text
CLONE_NEWPID
CLONE_NEWNET
CLONE_NEWNS
CLONE_NEWUSER
```

Damit kann ein neuer Task direkt in neuen Namespaces gestartet werden.

Vereinfacht:

```text
clone()
  |
  +--> neuer PID Namespace
  +--> neuer Network Namespace
  +--> neuer Mount Namespace
```

Das ist eine wichtige Grundlage für Container-Runtimes.

## Kurzvergleich

| `fork()` | `clone()` |
|---|---|
| klassischer neuer Prozess | flexibler neuer Task |
| weitgehend getrennte Ressourcen | Ressourcen gezielt teilbar |
| einfache POSIX-Schnittstelle | Linux-spezifischer |
| nutzt Copy-on-Write | kann Speicher direkt teilen |
| oft mit `exec()` kombiniert | Grundlage für Threads und Namespaces |

## Typisches Gesamtbild

```text
                Task-Erzeugung
                      |
          +-----------+-----------+
          |                       |
        fork()                  clone()
          |                       |
          v                       v
   neuer Prozess        Ressourcen gezielt teilen
          |                       |
          |                 +-----+------+
          |                 |            |
          v                 v            v
       exec()            Threads     Namespaces
```

## Merksatz

> **`fork()` erzeugt einen klassischen Kindprozess mit weitgehend eigener Prozesssicht und Copy-on-Write-Speicher. `clone()` ist flexibler und bestimmt über Flags, welche Ressourcen geteilt oder getrennt werden – dadurch bildet es eine wichtige Grundlage für Threads, Namespaces und Container.**
