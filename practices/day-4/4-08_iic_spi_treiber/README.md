# 4.08 - I²C/SPI-Treiber: Matching, Probe und Bus-Integration

## Ziel

Diese Übung zeigt den Unterschied zwischen einem I²C-Treiber und einem SPI-Treiber und wie ein Bus-Treiber mit einem realen Gerät über `probe()`, `remove()`, Device IDs und Device Tree-Matching verbunden wird.

Die Aufgabe ist bewusst klein gehalten und fokussiert sich auf die Struktur eines Bus-Treibers, nicht auf Hardware-Registerzugriffe.

## Lernziele

- I²C- und SPI-Treiber voneinander unterscheiden
- `probe()` und `remove()` verstehen
- `id_table` und `of_match_table` nutzen
- Matching auf der Bus-Ebene nachvollziehen
- die Rolle eines Treibers im Gerätetreiber-Stack erkennen

## Aufbau der Übung

Das Modul registriert zwei Treiber:

1. `demo_i2c_driver`
   - verwendet `struct i2c_driver`
   - matcht per `i2c_device_id` oder Device Tree-`compatible`
   - erkennt Geräte mit Namen wie `demo-i2c-sensor`
   - erwartet im Device Tree `compatible = "demo,i2c-sensor"`

2. `demo_spi_driver`
   - verwendet `struct spi_driver`
   - matcht per `spi_device_id` und optional per Device Tree
   - erkennt Geräte mit Namen wie `demo-spi-sensor`
   - erwartet im Device Tree `compatible = "demo,spi-sensor"`

## Bauen

```bash
make
```

## Typische Aufgaben

### Aufgabe 1: Treiber registrieren

```bash
sudo insmod training_i2c_spi.ko
sudo dmesg | tail -n 30
```

Fragen:

- Was passiert beim Laden des Moduls?
- Warum gibt es hier zwei separate Bus-Treiber?
- Welche Log-Ausgaben erwarten Sie in `dmesg`?

### Aufgabe 2: Matching verstehen

Analysieren Sie die Strukturen:

- `i2c_device_id`
- `spi_device_id`
- `of_device_id`
- `probe()`
- `remove()`

Fragen:

- Wie erkennt der Kernel, welches Gerät zu welchem Treiber passt?
- Warum ist `id_table` wichtig?
- Warum lohnt sich ein Device Tree-Check für SPI?

Prüfen Sie die vom Modul erzeugten Hardware-Aliase:

```bash
modinfo training_i2c_spi.ko | grep -E 'alias:.*(demo|i2c|spi)'
```

Erwartet werden unter anderem Aliase für:

- `demo,i2c-sensor`
- `demo,spi-sensor`
- `demo-i2c-sensor`
- `demo-spi-sensor`

Damit ist geprüft, dass Kbuild die `MODULE_DEVICE_TABLE()`-Einträge in die
Modulmetadaten übernimmt. Das tatsächliche `probe()` benötigt zusätzlich ein
registriertes I²C- bzw. SPI-Gerät mit passender Device-Tree-Beschreibung.

Beispielhafte Device-Tree-Knoten:

```dts
i2c_sensor@48 {
   compatible = "demo,i2c-sensor";
   reg = <0x48>;
};

spi_sensor@0 {
   compatible = "demo,spi-sensor";
   reg = <0>;
};
```

### Aufgabe 3: Logik in `probe()`

Der `probe()`-Callback ist der zentrale Einstiegspunkt für die Initialisierung. Er sollte:

- die Hardware-Umgebung prüfen
- Ressourcen reservieren
- Strukturen vorbereiten
- den Treiber mit dem Gerät verbinden

Fragen:

- Wofür wird `dev_info()` in `probe()` genutzt?
- Welche Informationen aus dem Gerät sollten bei der Initialisierung ausgewertet werden?
- Was passiert, wenn `probe()` fehlschlägt?

### Aufgabe 4: Unterschiede zu Platform-Treiber

Vergleichen Sie dieses Beispiel mit der vorigen Übung zum Platform-Device-Model.

Fragen:

- Wann verwendet man `platform_driver` und wann `i2c_driver` bzw. `spi_driver`?
- Wie sieht der Bus-Kontext aus?
- Welche Voraussetzung muss die Hardware erfüllen, damit das Matching funktioniert?

### Aufgabe 5: Reflektion

Schreiben Sie zwei bis drei Sätze zu den wichtigsten Unterschieden:

- I²C und SPI sind Bus-basierte Treiber.
- Die Auswahl des passenden Treibers geschieht über Matching-Mechanismen.
- Das eigentliche Produkt des Treibers ist die Hardware-Initialisierung und ein stabiler Lebenszyklus.

## Erwartete Ergebnisse

- Das Modul baut fehlerfrei.
- Beim Laden des Moduls werden die Treiber registriert.
- `modinfo` zeigt die I²C-, SPI- und Device-Tree-Aliase.
- Ein passendes I²C- oder SPI-Gerät auf einer passenden Plattform löst `probe()` aus.
- Bei Entfernen des Geräts oder beim Entfernen des Moduls läuft `remove()` bzw. der Unregister-Pfad.

## Musterlösung

- `i2c_add_driver()` registriert einen I²C-Treiber auf dem I²C-Bus.
- `spi_register_driver()` registriert einen SPI-Treiber auf dem SPI-Bus.
- Der Kernel vergleicht Namen bzw. Device IDs mit den von der Hardware beschriebenen Informationen.
- `probe()` initialisiert die Verbindung, `remove()` räumt auf.
- Device Tree- und Board-Informationen sind für `of_match_table` und `compatible` entscheidend.

## Hinweise

- Dieses Beispiel ist bewusst künstlich und keine reale Sensor-Implementierung.
- In einer echten Hardware-Umgebung muss das passende I²C-/SPI-Gerät im Device Tree, in ACPI oder in Board-Code vorhanden sein.
- Das Modul dient als Übung zur Struktur und zum Verständnis der Bus-Treiber-Architektur.

## Abschluss

Nach dieser Übung sollten Teilnehmende in der Lage sein, die Grundidee eines I²C- oder SPI-Treibers in eigenen Worten zu erklären: Ein Bus-Treiber beschreibt, wie ein Gerät auf einem bestimmten Bus erkannt und initialisiert wird. Die Entscheidung, ob ein Gerät und ein Treiber zusammenpassen, geschieht durch Matching, und die eigentliche Initialisierung erfolgt in `probe()`.
