# 4.04 - Device Model: Platform-Gerät und Platform-Treiber

## Ziel

Diese Übung zeigt die Grundprinzipien des Linux Device Models anhand eines künstlichen Platform-Geräts und eines passenden Platform-Treibers. Ziel ist es, Matching, Lebenszyklus und sysfs-Beobachtung nachvollziehbar zu machen.

## Lernziele

- Unterschied zwischen `platform_device` und `platform_driver` verstehen
- das Matching zwischen Gerät und Treiber erklären
- `probe()` und `remove()` beobachten
- die Bindung in sysfs nachvollziehen
- Fehlerfälle in der Initialisierung analysieren

## Verzeichnis

```text
4-04_device_model/
├── Makefile
├── README.md
├── training_device.c
├── training_driver.c
└── run.sh
```

## Aufbau der Übung

Es gibt zwei Module:

1. `training_device.ko`
   - registriert ein künstliches Platform-Gerät namens `training-led`
2. `training_driver.ko`
   - registriert einen Treiber mit demselben Namen
   - bindet sich an das Gerät und ruft `probe()` auf

Die Aufgaben zeigen, wie der Platform-Bus Gerät und Treiber verbindet und welche Zustände sich in sysfs und in `dmesg` beobachten lassen.

## Bauen

```bash
make
```

## Aufgaben

### Aufgabe 1: Gerät registrieren

```bash
sudo insmod training_device.ko
ls -l /sys/bus/platform/devices/
ls -l /sys/bus/platform/devices/training-led/
readlink /sys/bus/platform/devices/training-led/driver || echo "kein Treiber gebunden"
```

Fragen:

- Wo erscheint das neue Gerät?
- Ist bereits ein Treiber-Link vorhanden?
- Warum fehlt der Link noch?

### Aufgabe 2: Treiber registrieren

```bash
sudo insmod training_driver.ko
sudo dmesg | tail -n 30
readlink /sys/bus/platform/devices/training-led/driver
ls -l /sys/bus/platform/drivers/training-led/
```

Fragen:

- Was passiert beim Laden des Treibers?
- Warum wird `probe()` aufgerufen?
- Was zeigt der `driver`-Link im Gerät?

### Aufgabe 3: Lebenszyklus beobachten

```bash
echo training-led | sudo tee /sys/bus/platform/drivers/training-led/unbind
sudo dmesg | tail -n 20

echo training-led | sudo tee /sys/bus/platform/drivers/training-led/bind
sudo dmesg | tail -n 20
```

Fragen:

- Welche Callback-Funktion läuft beim `unbind`?
- Welches Verhalten zeigt `bind`?
- Warum ist sysfs für die Bindung wichtig?

### Aufgabe 4: Fehlerfall in `probe()`

```bash
sudo rmmod training_driver
sudo insmod training_driver.ko fail_probe=1
sudo dmesg | tail -n 20
readlink /sys/bus/platform/devices/training-led/driver || echo "kein Treiber gebunden"
```

Fragen:

- Warum wird die Bindung nicht hergestellt, wenn `probe()` fehlschlägt?
- Warum erscheint kein `driver`-Link?
- Warum läuft `remove()` in diesem Fall nicht?

### Aufgabe 5: Systematische Auswertung

Schreibt zu Ihrer Beobachtung drei Kernpunkte auf:

- Ein `platform_device` beschreibt ein Hardware-Objekt.
- Ein `platform_driver` beschreibt das Verhalten dafür.
- `probe()` entscheidet darüber, ob Gerät und Treiber verbunden werden.

## Erwartete Ergebnisse

Typischer Verlauf:

1. Nach dem Laden des Geräts: Gerät sichtbar, aber kein Treiber gebunden.
2. Nach dem Laden des Treibers: `probe()` wird aufgerufen und die Bindung entsteht.
3. Nach `unbind`: `remove()` läuft.
4. Nach `bind`: `probe()` läuft erneut.
5. Bei `fail_probe=1`: Bindung wird verweigert und Fehler erscheint in `dmesg`.

## Musterlösung

- Der Platform-Bus matcht Gerät und Treiber per Namen.
- `probe()` initialisiert das Gerät nur dann, wenn die Bindung erfolgreich ist.
- Beim `unbind` wird die Bindung entfernt und `remove()` aufgerufen.
- Ein fehlerhaftes `probe()` verhindert die Bindung vollständig.
- sysfs zeigt den aktuellen Zustand der Gerät-/Treiber-Bindung.

## Hinweise

- `platform_device` und `platform_driver` sind die zentralen Objekte des Platform Device Model.
- Das Beispiel ist bewusst künstlich und dient nur der Veranschaulichung.
- Keine dieser Module sollte in einer produktiven Umgebung ohne Absicherung geladen werden.

## Optional: Reibungsloses Test-Script

Ein kleines Test-Skript kann die Übung automatisieren:

```bash
#!/bin/bash
set -e

make
sudo insmod training_device.ko
sudo insmod training_driver.ko
sudo dmesg | tail -n 20
readlink /sys/bus/platform/devices/training-led/driver
sudo rmmod training_driver
sudo rmmod training_device
```

## Abschluss

Nach dieser Übung sollten die Teilnehmenden in der Lage sein, das Grundprinzip des Linux Device Models in Worten zu erklären: Ein Gerät beschreibt ein vorhandenes Objekt, ein Treiber beschreibt, wie man es steuert, und der Bus verbindet beides nur dann, wenn ein passender Match und eine erfolgreiche Initialisierung vorliegen.
