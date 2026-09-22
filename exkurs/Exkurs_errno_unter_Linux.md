# Exkurs: `errno` unter Linux

## Was ist `errno`?

`errno` ist ein standardisierter Mechanismus zur **Fehlerkennzeichnung in Userspace-Programmen**.

Viele C-Bibliotheksfunktionen und System-Call-Wrapper liefern bei einem Fehler einen speziellen Rückgabewert zurück, zum Beispiel:

```c
-1
```

Zusätzlich wird `errno` auf einen Fehlercode gesetzt.

Beispiel:

```c
int fd = open("datei.txt", O_RDONLY);

if (fd == -1) {
    printf("Fehlercode: %d\n", errno);
}
```

Wichtig:

> `errno` ist keine Funktion des Linux-Kernels selbst, sondern Teil der Userspace-Schnittstelle, typischerweise über die libc.

## Typischer Ablauf

Ein Programm ruft beispielsweise auf:

```c
read(fd, buffer, size);
```

Vereinfacht passiert:

```text
Userspace
   |
   | read()
   v
libc
   |
   | System Call
   v
Linux Kernel
   |
   +--> Erfolg
   |      |
   |      v
   |   Rückgabewert >= 0
   |
   +--> Fehler
          |
          v
      negativer Fehlercode
          |
          v
libc setzt errno
```

Der Kernel liefert Fehler intern als **negative Werte** zurück, zum Beispiel:

```text
-ENOENT
-EINVAL
-ENOMEM
```

Die libc übersetzt das typischerweise in:

```text
Rückgabewert = -1
errno        = Fehlernummer
```

## Beispiel

Angenommen, eine Datei existiert nicht:

```c
int fd = open("/tmp/nicht-da", O_RDONLY);
```

Der Kernel kann intern zurückgeben:

```text
-ENOENT
```

Die libc macht daraus für das Programm:

```text
fd = -1
errno = ENOENT
```

`ENOENT` bedeutet:

```text
No such file or directory
```

## Häufige Fehlercodes

| Symbol | Bedeutung |
|---|---|
| `ENOENT` | Datei oder Verzeichnis existiert nicht |
| `EACCES` | Zugriff verweigert |
| `EPERM` | Operation nicht erlaubt |
| `EINVAL` | ungültiges Argument |
| `ENOMEM` | nicht genügend Speicher |
| `EBUSY` | Ressource ist belegt |
| `EEXIST` | Objekt existiert bereits |
| `ENOSPC` | kein Speicherplatz verfügbar |
| `EINTR` | System Call wurde durch Signal unterbrochen |
| `EAGAIN` | Ressource momentan nicht verfügbar |
| `EBADF` | ungültiger File Descriptor |

## `errno` ausgeben

Zur Ausgabe einer lesbaren Fehlermeldung kann `perror()` verwendet werden:

```c
if (fd == -1) {
    perror("open");
}
```

Ausgabe:

```text
open: No such file or directory
```

Alternativ:

```c
printf("%s\n", strerror(errno));
```

## Wichtig: `errno` nur bei Fehler prüfen

`errno` sollte nur ausgewertet werden, wenn die aufgerufene Funktion tatsächlich einen Fehler gemeldet hat.

Falsch:

```c
read(fd, buffer, size);

if (errno != 0) {
    ...
}
```

Richtig:

```c
ssize_t ret = read(fd, buffer, size);

if (ret == -1) {
    printf("%s\n", strerror(errno));
}
```

Grund:

> Erfolgreiche Funktionsaufrufe müssen `errno` nicht auf `0` zurücksetzen.

Ein alter Fehlerwert kann also stehen bleiben.

## Kernel-Seite: negative Fehlercodes

Im Kernel-Code werden Fehler meist direkt als negative Werte zurückgegeben.

Beispiel:

```c
if (!ptr)
    return -ENOMEM;
```

oder:

```c
if (invalid)
    return -EINVAL;
```

Typisches Muster:

```text
Kernel:
return -EINVAL;

Userspace:
return -1;
errno = EINVAL;
```

## Warum negative Fehlerwerte?

Viele Kernel-Funktionen liefern normalerweise:

- `0` für Erfolg
- positive Werte für Datenmengen, IDs oder andere Ergebnisse

Negative Werte können daher eindeutig als Fehler interpretiert werden.

Beispiel:

```text
 42       -> gültiger Rückgabewert
  0       -> Erfolg
-22       -> Fehler
```

`EINVAL` hat unter Linux typischerweise den Wert `22`:

```text
Kernel intern: -EINVAL = -22
Userspace:      errno = 22
```

## Fehlercodes bei Zeigern

Kernel-Code verwendet häufig auch sogenannte **Error Pointers**.

Statt:

```c
return NULL;
```

kann eine Funktion einen Fehler in einem Pointer kodieren:

```c
return ERR_PTR(-ENOMEM);
```

Der Aufrufer prüft dann:

```c
if (IS_ERR(ptr))
    return PTR_ERR(ptr);
```

Typische Makros:

```text
ERR_PTR()
IS_ERR()
PTR_ERR()
```

Dieses Muster findet man häufig in Kernel-Subsystemen und Treibern.

## `EINTR` und unterbrochene System Calls

Ein besonderer Fehler ist `EINTR`. Er kann entstehen, wenn ein blockierender System Call durch ein Signal unterbrochen wird.

```text
read()
   |
   | wartet
   |
Signal trifft ein
   |
   v
read() -> -1
errno  -> EINTR
```

Je nach Situation kann ein Programm den System Call erneut ausführen.

## `EAGAIN` und `EWOULDBLOCK`

Bei nicht blockierendem I/O kann ein System Call `EAGAIN` oder `EWOULDBLOCK` melden.

```text
Socket ist non-blocking
      |
      v
keine Daten vorhanden
      |
      v
read() = -1
errno  = EAGAIN
```

Das bedeutet nicht zwingend einen dauerhaften Fehler, sondern: **Die Operation kann im Moment nicht ausgeführt werden.**

## Wo sind Fehlercodes definiert?

Userspace-Programme verwenden typischerweise:

```c
#include <errno.h>
```

Im Kernel gibt es entsprechende Definitionen in Kernel-Headern.

Beispiele:

```text
EINVAL
ENOMEM
ENOENT
EIO
EPERM
```

Die symbolischen Namen sind wesentlich aussagekräftiger als reine Zahlen.

## Merksatz

> **Der Linux-Kernel liefert Fehler intern typischerweise als negative Fehlercodes wie `-EINVAL` oder `-ENOMEM`; die libc übersetzt diese für Userspace-Programme meist in einen Rückgabewert von `-1` und setzt dazu `errno` auf den entsprechenden positiven Fehlercode.**
