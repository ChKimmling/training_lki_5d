# Exkurs: VFS-Objekte im Linux-Kernel

## Was ist das VFS?

Das **VFS (Virtual File System)** ist die Abstraktionsschicht des Linux-Kernels für Dateisysteme.

Es sorgt dafür, dass Anwendungen dieselben System Calls verwenden können, unabhängig davon, ob eine Datei z. B. auf `ext4`, `xfs`, `tmpfs` oder einem anderen Dateisystem liegt.

```text
Userspace
   |
   | open(), read(), write(), stat()
   v
+----------------------+
|         VFS          |
+----------------------+
   |        |       |
  ext4     xfs    tmpfs
```

Für die Verwaltung verwendet das VFS mehrere zentrale Objekte.

Die wichtigsten sind:

- **Superblock**
- **Inode**
- **Dentry**
- **File**

---

## Superblock

Der **Superblock** beschreibt ein eingebundenes Dateisystem als Ganzes.

Er enthält Informationen wie:

- Dateisystemtyp
- Mount-bezogene Zustände
- Blockgröße
- Verwaltungsinformationen
- Referenzen auf dateisystemspezifische Operationen
- Root-Verzeichnis des Dateisystems

Vereinfacht:

```text
Superblock
   |
   +--> beschreibt ein Dateisystem
   |
   +--> z. B. ext4 auf /dev/sda1
```

Ein gemountetes Dateisystem besitzt im VFS typischerweise einen zugehörigen Superblock.

Kernel-Struktur:

```c
struct super_block
```

---

## Inode

Der **Inode** beschreibt ein Dateisystemobjekt.

Das kann zum Beispiel sein:

- reguläre Datei
- Verzeichnis
- Symbolic Link
- Device Node
- Socket
- FIFO

Ein Inode enthält unter anderem Metadaten wie:

- Dateityp
- Dateigröße
- Besitzer (`UID`)
- Gruppe (`GID`)
- Zugriffsrechte
- Zeitstempel
- Blockzuordnung
- dateisystemspezifische Operationen

Wichtig:

> Der Inode enthält normalerweise **nicht den Dateinamen**.

Beispiel:

```text
Inode
  |
  +-- Größe
  +-- Rechte
  +-- UID/GID
  +-- Zeitstempel
  +-- Datenblöcke
```

Kernel-Struktur:

```c
struct inode
```

---

## Dentry

Ein **Dentry (Directory Entry)** verbindet einen **Dateinamen** mit einem Inode.

Beispiel:

```text
/etc/passwd
```

kann vereinfacht zerlegt werden in:

```text
"/"
 |
 +--> "etc"
       |
       +--> "passwd"
```

Für diese Pfadbestandteile verwendet das VFS Dentry-Objekte.

Ein Dentry enthält unter anderem:

- Namen eines Pfadbestandteils
- Referenz auf den zugehörigen Inode
- Referenz auf das Parent-Dentry
- Informationen für die Dentry-Cache-Verwaltung

```text
Dentry "passwd"
      |
      +--> Inode 12345
```

Kernel-Struktur:

```c
struct dentry
```

### Dentry Cache

Linux hält bereits aufgelöste Pfadbestandteile im **Dentry Cache (dcache)**.

Dadurch muss nicht bei jedem Zugriff der gesamte Dateipfad erneut vom Dateisystem aufgelöst werden.

---

## File

Das **File-Objekt** repräsentiert eine **geöffnete Datei**.

Es entsteht typischerweise durch:

```c
open()
```

oder:

```c
openat()
```

Ein `struct file` enthält Informationen zum aktuellen geöffneten Zugriff, zum Beispiel:

- aktuelle Dateiposition
- Öffnungsflags
- Zugriffsmodus
- Referenz auf den Pfad
- Referenz auf Dateioperationen

Kernel-Struktur:

```c
struct file
```

Wichtig:

> `struct file` beschreibt nicht die Datei auf dem Datenträger, sondern eine **konkrete geöffnete Instanz** dieser Datei.

Zwei Prozesse können daher dieselbe Datei über unterschiedliche `struct file`-Objekte geöffnet haben.

---

## Zusammenspiel der VFS-Objekte

Beispiel:

```text
open("/home/user/test.txt")
```

Vereinfacht passiert:

```text
Pfadname
   |
   v
Dentry-Auflösung
   |
   v
Inode finden
   |
   v
File-Objekt erzeugen
   |
   v
File Descriptor zurückgeben
```

Die Beziehungen können vereinfacht so dargestellt werden:

```text
Superblock
    |
    +--> Inodes
           |
           +--> Inode
                  ^
                  |
                Dentry
                  ^
                  |
              struct file
```

Etwas genauer:

```text
Prozess
   |
   | fd = 3
   v
File Descriptor Table
   |
   v
struct file
   |
   +--> struct path
   |       |
   |       +--> dentry
   |               |
   |               +--> inode
   |
   +--> file_operations
```

---

## Beispiel mit zwei Prozessen

Zwei Prozesse öffnen dieselbe Datei:

```text
Prozess A                   Prozess B
   |                           |
   | fd 3                      | fd 5
   v                           v
struct file A              struct file B
       \                     /
        \                   /
         +---- Dentry ------+
                  |
                  v
               Inode
                  |
                  v
              Superblock
```

Damit können beide Prozesse:

- dieselbe Datei referenzieren
- unterschiedliche Dateipositionen besitzen
- unterschiedliche Open-Flags verwenden

---

## File Descriptor vs. `struct file`

Ein **File Descriptor** ist lediglich eine kleine Zahl im Userspace:

```text
0 = stdin
1 = stdout
2 = stderr
3 = erste weitere geöffnete Datei
```

Der File Descriptor verweist innerhalb des Prozesses auf ein `struct file`.

```text
fd 3
 |
 v
struct file
 |
 v
dentry
 |
 v
inode
```

Der File Descriptor selbst ist also **kein VFS-Objekt**.

---

## Kurzüberblick

| VFS-Objekt | Bedeutung |
|---|---|
| `super_block` | beschreibt ein eingebundenes Dateisystem |
| `inode` | beschreibt ein Dateisystemobjekt und dessen Metadaten |
| `dentry` | verbindet einen Namen/Pfadbestandteil mit einem Inode |
| `file` | beschreibt eine konkrete geöffnete Datei |

---

## Merksatz

> **Superblock = Dateisystem, Inode = Dateiobjekt, Dentry = Name/Pfad, File = geöffnete Instanz.**

Oder als kompakte Kette:

```text
Dateisystem
    |
Superblock
    |
  Inode
    ^
    |
  Dentry
    ^
    |
   File
    ^
    |
File Descriptor
```
