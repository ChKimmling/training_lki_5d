# Übung: Platform-Treiber und Matching

In dieser Übung wird ein künstliches Platform-Gerät erzeugt und mit einem passenden Treiber verbunden. Ziel ist es, das Verhalten von `platform_device`, `platform_driver`, `probe()`, `remove()` und sysfs nachzuvollziehen.

## Lernziele

- Unterschied zwischen Platform-Gerät und Platform-Treiber erklären
- Matching zwischen Gerät und Treiber nachvollziehen
- `probe()` und `remove()` beobachten
- Bindung in sysfs analysieren
- Fehlerfall in `probe()` verstehen

## Vorbereitung

- Linux-System oder VM mit Kernel-Build-Verzeichnis
- `build-essential` und passende Kernel-Header installiert
- Root-Rechte für `insmod`, `rmmod`, `dmesg` und sysfs-Checks

> Hinweis: Der Build und die Übersetzung funktionieren ohne Root. Der tatsächliche Laufzeit-Test mit `insmod` erfordert aber Admin-Rechte.

## Verzeichnisstruktur

```text
platform_treiber/
├── Makefile
├── README.md
├── training_device.c
└── training_driver.c
```

## 1. Geräte-Modul bauen und laden

Das erste Modul erzeugt ein künstliches Platform-Gerät mit Namen `training-led`.

```bash
make
sudo insmod training_device.ko
ls -l /sys/bus/platform/devices/
ls -l /sys/bus/platform/devices/training-led/
readlink /sys/bus/platform/devices/training-led/driver || echo "kein Treiber gebunden"
```

### Aufgaben

1. Prüfe, ob das Gerät in `/sys/bus/platform/devices/` erscheint.
2. Kontrolliere, ob ein `driver`-Link existiert.
3. Notiere die Beobachtung.

### Erwartete Beobachtung

Das Gerät ist sichtbar, aber es gibt noch keinen `driver`-Link, weil noch kein passender Treiber registriert ist.

---

## 2. Treiber-Modul laden

Jetzt wird der Treiber geladen.

```bash
sudo insmod training_driver.ko
sudo dmesg | tail -n 30
readlink /sys/bus/platform/devices/training-led/driver
ls -l /sys/bus/platform/drivers/training-led/
```

### Aufgaben

1. Beobachte die Kernel-Ausgabe.
2. Prüfe, ob `probe()` aufgerufen wurde.
3. Kontrolliere, ob der `driver`-Link im Gerät entsteht.

### Erwartete Beobachtung

Die Gerätedaten und der Treibername passen zueinander. Der Platform-Bus matcht Gerät und Treiber und ruft `probe()` auf.

---

## 3. Lebenszyklus beobachten

### `unbind` und `bind`

```bash
echo training-led | sudo tee /sys/bus/platform/drivers/training-led/unbind
sudo dmesg | tail -n 20

echo training-led | sudo tee /sys/bus/platform/drivers/training-led/bind
sudo dmesg | tail -n 20
```

### Aufgaben

1. Welche Callback-Funktion wird beim `unbind` aufgerufen?
2. Wie verhält sich `bind` im Vergleich dazu?
3. Welche Rolle spielt sysfs dabei?

### Erwartete Beobachtung

Beim `unbind` wird `remove()` ausgeführt. Beim `bind` läuft `probe()` erneut.

---

## 4. Fehlerfall in `probe()`

Der Treiber unterstützt einen Fehlerpfad, der `probe()` absichtlich fehlschlagen lässt.

```bash
sudo rmmod training_driver
sudo insmod training_driver.ko fail_probe=1
sudo dmesg | tail -n 30
readlink /sys/bus/platform/devices/training-led/driver || echo "kein Treiber gebunden"
```

### Aufgaben

1. Was passiert, wenn `probe()` mit `-EIO` fehlschlägt?
2. Warum entsteht dann kein `driver`-Link?
3. Warum wird `remove()` in diesem Fall nicht aufgerufen?

### Erwartete Beobachtung

Ein fehlgeschlagener `probe()` verhindert die Bindung zwischen Gerät und Treiber. Der Fehler bleibt in `dmesg` sichtbar.

---

## 5. Abschlussaufgaben

### Aufgabe A: Warum ist das Gerät am Anfang ungebunden?

Erkläre den Zustand vor dem Laden des Treibers.

### Aufgabe B: Warum wird `probe()` ausgeführt?

Beschreibe den Matching-Vorgang zwischen Gerät und Treiber.

### Aufgabe C: Was zeigt sysfs?

Was bedeutet ein vorhandener oder fehlender `driver`-Link?

### Aufgabe D: Warum ist `probe()` ein Erfolgs- oder Fehler-Check?

Diskutiere die Rolle von `probe()` für die Geräteinitialisierung.

---

## Musterlösung

### 1. Warum ist das Gerät am Anfang ungebunden?

Das Gerät wird registriert, aber es gibt noch keinen passenden Treiber. Der Platform-Bus kann daher keine Bindung herstellen.

### 2. Warum wird `probe()` ausgeführt?

Die Namen von Gerät und Treiber stimmen überein. Dadurch erkennt der Platform-Bus den passenden Treiber und ruft `probe()` auf.

### 3. Warum ist `remove()` nach `unbind` sichtbar?

`unbind` entfernt die Bindung zwischen Gerät und Treiber und löst damit den Treiber-Lebenszyklus für dieses Gerät aus.

### 4. Was passiert bei einem fehlerhaften `probe()`?

Die Initialisierung schlägt fehl. Der Treiber wird nicht erfolgreich an das Gerät gebunden, und der Fehler bleibt sichtbar.

### 5. Was ist die zentrale Idee dieser Übung?

Ein Platform-Gerät beschreibt das vorhandene Gerät, ein Platform-Treiber beschreibt das Verhalten für passende Geräte. Erst das erfolgreiche Matching und die erfolgreiche Initialisierung verbinden beides.

---

## Abschluss

Schreibt drei Kernsätze auf:

- Ein Platform-Gerät beschreibt die Hardwareinstanz.
- Ein Platform-Treiber beschreibt das Verhalten für das passende Gerät.
- `probe()` entscheidet, ob die Bindung erfolgreich ist.

Damit habt ihr das grundlegende Verständnis für das Platform Device Model und das Matching im Linux-Kernel erarbeitet.
