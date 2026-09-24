# Exkurs: Major- und Minor-Nummern unter Linux

## Wozu dienen sie?

Gerätedateien wie `/dev/null` oder `/dev/sda` sind **Character Devices** bzw. **Block Devices**. Statt gewöhnlicher Dateiinhalte besitzen sie eine **Major- und eine Minor-Nummer**. Beim Öffnen einer Gerätedatei verwendet Linux diese Kennung, um das zuständige Gerät bzw. den registrierten Treiber zu finden.

| Nummer | Aufgabe |
|---|---|
| **Major** | Identifiziert einen Gerätetreiber bzw. eine registrierte Geräteklasse innerhalb von Character- oder Block-Devices. |
| **Minor** | Unterscheidet Geräte oder Instanzen innerhalb des Bereichs einer Major-Nummer; die Bedeutung legt der Treiber fest. |

Beispiel: `/dev/null` ist ein Character Device mit **Major 1, Minor 3**. Eine identische Zahlenkombination kann im getrennten Namensraum der Block Devices eine andere Bedeutung haben.

```text
open("/dev/demo0")
        │
        ▼
Gerätedatei (Character Device)
        │  Major = 240, Minor = 0  (nur Beispiel)
        ▼
registriertes Gerät / cdev
        │
        ▼
Treiber: file_operations → open(), read(), write() …
```

## Im laufenden System untersuchen

```bash
ls -l /dev/null          # crw-rw-rw- ... 1, 3 ... /dev/null
stat -c '%F: major=%t minor=%T' /dev/null  # Major/Minor in Hex
cat /proc/devices        # Registrierte Character- und Block-Major-Nummern
ls /sys/dev/char/        # Zugeordnete Character-Device-Nummern
ls /sys/dev/block/       # Zugeordnete Block-Device-Nummern
```

Das führende **`c`** bei `ls -l` bezeichnet ein Character Device, **`b`** ein Block Device. Die Nummern erscheinen anstelle der gewöhnlichen Dateigröße.

## Kernel-API: `dev_t`

Der Kernel fasst beide Nummern in `dev_t` zusammen. Die Makros `MKDEV()`, `MAJOR()` und `MINOR()` erzeugen bzw. zerlegen diese Kennung.

```c
#include <linux/kdev_t.h>

dev_t dev = MKDEV(240, 0);
unsigned int major = MAJOR(dev);
unsigned int minor = MINOR(dev);
```

Bei einem Character-Treiber werden Major-/Minor-Nummern häufig **dynamisch** reserviert:

```c
#include <linux/fs.h>
#include <linux/cdev.h>

static dev_t devno;
static struct cdev demo_cdev;

/* Initialisierung – Fehlerbehandlung vollständig ergänzen */
ret = alloc_chrdev_region(&devno, 0, 1, "demo");
if (ret)
    return ret;

cdev_init(&demo_cdev, &demo_fops);
ret = cdev_add(&demo_cdev, devno, 1);
if (ret) {
    unregister_chrdev_region(devno, 1);
    return ret;
}

/* Beim Entfernen: cdev_del(&demo_cdev); unregister_chrdev_region(devno, 1); */
```

`alloc_chrdev_region()` reserviert einen Nummernbereich; `cdev_add()` verbindet ihn mit den `file_operations`. Ein `/dev`-Eintrag entsteht dadurch **nicht automatisch**. Üblich sind `class_create()` und `device_create()` in Verbindung mit **devtmpfs/udev**; manuell lässt sich ein Eintrag mit `mknod` erstellen:

```bash
sudo mknod /dev/demo0 c <MAJOR> 0
```

## Wichtig zu unterscheiden

- **Major/Minor** kennzeichnen das Gerät in einer Gerätedatei; sie sind **keine File Descriptors**. Ein Prozess kann z. B. `fd = 3` für `/dev/null` erhalten, dessen Gerätenummer weiterhin `1:3` ist.
- Die Nummern allein definieren **keine Zugriffsrechte**: Dateiberechtigungen, LSM und weitere Kernelprüfungen gelten weiterhin.
- Für neue Treiber ist die **dynamische Major-Vergabe** meist sinnvoller als eine willkürlich festgelegte Major-Nummer.

**Merksatz:** *Major wählt den registrierten Gerätebereich, Minor unterscheidet die zugehörigen Geräteinstanzen – beide werden im Kernel als `dev_t` verwaltet.*
