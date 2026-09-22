# Exkurs: Linux ABI und System-Call-Nummern

## Was ist eine ABI?

**ABI** steht für **Application Binary Interface**.

Eine ABI beschreibt die binäre Schnittstelle zwischen kompiliertem Programm und Betriebssystem bzw. Laufzeitumgebung.

Sie legt unter anderem fest:

- wie Funktionen aufgerufen werden
- wo Argumente übergeben werden
- wie Rückgabewerte geliefert werden
- welche Register verwendet werden
- wie Datenstrukturen aufgebaut sind
- wie System Calls aufgerufen werden
- welche System-Call-Nummern gelten

Die ABI ist damit die Grundlage dafür, dass ein bereits kompiliertes Programm mit dem Linux-Kernel kommunizieren kann.

---

## API vs. ABI

Eine **API** beschreibt die Schnittstelle auf Quellcode-Ebene.

Beispiel:

```c
write(fd, buffer, len);
```

Eine **ABI** beschreibt dagegen, wie dieser Aufruf auf Maschinenebene umgesetzt wird.

```text
API
 |
 | C-Funktion write()
 v
libc
 |
 | ABI
 v
System Call
 |
 v
Linux Kernel
```

Kurz:

```text
API = Quellcode-Schnittstelle
ABI = Binär-Schnittstelle
```

---

## System Calls als Kernel-ABI

System Calls bilden eine zentrale Schnittstelle zwischen Userspace und Kernel.

Beispiele:

```text
read
write
openat
close
mmap
ioctl
clone
execve
```

Ein Userspace-Prozess kann Kernel-Funktionen nicht direkt aufrufen.

Stattdessen erfolgt der Übergang über eine definierte System-Call-ABI.

```text
Userspace
    |
    | System Call
    v
Kernel
```

---

## Warum gibt es System-Call-Nummern?

Der Kernel muss erkennen, **welcher System Call ausgeführt werden soll**.

Dazu besitzt jeder System Call innerhalb einer ABI eine Nummer.

Beispiel für Linux x86-64:

```text
read      -> 0
write     -> 1
close     -> 3
mmap      -> 9
```

Die Nummer wird beim System-Call-Aufruf in ein festgelegtes Register geschrieben.

Der Kernel verwendet sie anschließend als Kennung für den gewünschten System Call.

---

## Beispiel: `write()` auf x86-64

Ein Programm ruft auf:

```c
write(1, "Hallo\n", 6);
```

Die libc bereitet daraus den eigentlichen System Call vor.

Für die x86-64-System-Call-ABI gilt vereinfacht:

```text
RAX = System-Call-Nummer
RDI = Argument 1
RSI = Argument 2
RDX = Argument 3
R10 = Argument 4
R8  = Argument 5
R9  = Argument 6
```

Für `write()`:

```text
RAX = 1          syscall: write
RDI = 1          fd = stdout
RSI = Adresse    Buffer
RDX = 6          Länge
```

Danach führt die CPU die Instruktion aus:

```asm
syscall
```

Ablauf:

```text
Userspace
   |
   | RAX = 1
   | RDI = 1
   | RSI = buffer
   | RDX = 6
   |
   v
syscall
   |
   v
Kernel
   |
   v
write-System-Call
```

---

## Rückgabewert

Auf x86-64 wird der Rückgabewert typischerweise in:

```text
RAX
```

zurückgegeben.

Beispiel:

```text
RAX = 6
```

bedeutet bei `write()`:

```text
6 Bytes geschrieben
```

Bei einem Fehler liefert der Kernel intern einen negativen Fehlercode zurück.

Die libc übersetzt diesen typischerweise in:

```text
-1
```

und setzt:

```c
errno
```

entsprechend.

---

## Architekturabhängige ABI

Wichtig:

> System-Call-Nummern sind nicht auf allen CPU-Architekturen identisch.

Beispiel:

```text
x86-64
ARM64
RISC-V
x86-32
```

können unterschiedliche System-Call-Nummern und unterschiedliche Registerkonventionen verwenden.

Das bedeutet:

```text
gleicher System Call
!=
gleiche Nummer auf jeder Architektur
```

Die ABI ist also architekturspezifisch.

---

## System-Call-Tabelle

Der Linux-Kernel verwaltet System Calls über architekturspezifische Tabellen und Definitionen.

Für x86 findet man relevante Dateien beispielsweise unter:

```text
arch/x86/entry/syscalls/
```

Dort befinden sich unter anderem Tabellen wie:

```text
syscall_64.tbl
```

Vereinfacht enthalten sie Zuordnungen wie:

```text
Nummer    ABI      Name       Kernel-Entry
0         common   read       sys_read
1         common   write      sys_write
...
```

Die genaue Kernelstruktur kann sich intern ändern, die Userspace-ABI soll dagegen stabil bleiben.

---

## Warum ist ABI-Stabilität wichtig?

Ein bereits kompiliertes Programm kennt die ABI, gegen die es gebaut wurde.

Wenn sich bestehende System-Call-Nummern oder deren Bedeutung beliebig ändern würden, könnten alte Programme nicht mehr funktionieren.

Deshalb ist die Userspace-ABI unter Linux besonders wichtig:

```text
altes Programm
     |
     | gleiche ABI
     v
neuer Linux-Kernel
```

Ein neuer Kernel soll alte Userspace-Programme weiterhin ausführen können.

---

## libc zwischen Anwendung und Kernel

Anwendungen rufen System Calls meist nicht direkt auf.

Stattdessen verwenden sie Bibliotheken wie:

```text
glibc
musl
```

Beispiel:

```text
Anwendung
   |
   | write()
   v
glibc
   |
   | System-Call-ABI
   v
Linux Kernel
```

Die libc übernimmt dabei unter anderem:

- Registerbelegung
- System-Call-Nummer
- CPU-Instruktion für den Übergang
- Fehlerbehandlung
- `errno`

---

## System Calls beobachten

Mit `strace` lässt sich die System-Call-Schnittstelle beobachten:

```bash
strace ./programm
```

Beispiel:

```text
write(1, "Hallo\n", 6) = 6
```

Damit sieht man die API-nahe Darstellung des tatsächlichen Übergangs zum Kernel.

---

## Syscall-Nummern anzeigen

Je nach System lassen sich Definitionen beispielsweise in Kernel-Headern finden.

Typisch auf x86-64:

```bash
grep __NR_write /usr/include/x86_64-linux-gnu/asm/unistd_64.h
```

Beispiel:

```c
#define __NR_write 1
```

Damit ist sichtbar:

```text
write -> System Call Nummer 1
```

---

## Zusammenfassung

```text
Anwendung
   |
   | API: write()
   v
libc
   |
   | ABI
   | RAX = Syscall-Nummer
   | Register = Argumente
   v
syscall-Instruktion
   |
   v
Linux Kernel
   |
   v
System-Call-Handler
```

Die ABI definiert dabei den gesamten binären Vertrag zwischen Userspace und Kernel.

---

## Merksatz

> **Die Linux-System-Call-ABI legt fest, wie ein Userspace-Programm den Kernel binär aufruft. Die System-Call-Nummer identifiziert dabei den gewünschten Kernel-Dienst, während Register die Argumente und Rückgabewerte transportieren.**
