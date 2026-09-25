# 5.06 - Power Management: Runtime-PM, System-Suspend und Wakeup

## Ziel

Diese Übung zeigt, wie ein Gerätetreiber Runtime-PM, System-Suspend und
Wakeup korrekt miteinander verbindet: eine balancierte PM-Referenz pro
Zugriff, eine feste Ressourcenreihenfolge in den Callbacks und Evidenz aus
sysfs/`dmesg` für den beobachteten Übergang.

## Lernziele

- den Runtime-PM-Zyklus (active → idle → suspended → resume) einordnen
- `pm_runtime_resume_and_get()`/`pm_runtime_put_autosuspend()` als
  symmetrisches Get/Put-Paar um jeden Gerätezugriff legen
- Autosuspend-Policy (`autosuspend_delay_ms`, `power/control`) beobachten
- die Wiederverwendung des Runtime-PM-Pfads für System-Sleep
  (`pm_runtime_force_suspend`/`_resume`) nachvollziehen
- Wakeup-Grundlagen (`device_init_wakeup`, `pm_wakeup_dev_event`) anhand
  von sysfs-Zählern beobachten

## Verzeichnis

```text
5-06_power-management/
├── Makefile
├── README.md
├── pm_demo_device.c
├── pm_demo_driver.c
└── run.sh
```

## Aufbau der Übung

Es gibt zwei Module:

1. `pm_demo_device.ko` – registriert ein künstliches Platform-Gerät namens
   `pm-demo` (wie in Übung 4.06).
2. `pm_demo_driver.ko` – bindet sich an `pm-demo` und implementiert:
   - **Runtime-PM**: `runtime_suspend`/`runtime_resume` schalten ein
     simuliertes Register (`shadow_reg`) ab bzw. restaurieren es;
     Autosuspend ist mit 2000 ms konfiguriert.
   - **System-Sleep**: `.suspend`/`.resume` zeigen auf
     `pm_runtime_force_suspend()`/`pm_runtime_force_resume()` und nutzen
     damit denselben Callback-Pfad wie Runtime-PM (Folie 09).
   - **Wakeup**: `device_init_wakeup()` in `probe()` und
     `pm_wakeup_dev_event()` über ein sysfs-Attribut simulieren ein
     Wakeup-Ereignis.
   - **sysfs-Attribute** `access` (write-only), `access_count` (read-only)
     und `simulate_wakeup` (write-only) machen den Zugriffspfad und das
     Wakeup-Accounting beobachtbar, ohne einen echten Bus anzusprechen.

## Bauen

```bash
make
```

## Aufgaben

### Aufgabe 1: Gerät und Treiber laden

```bash
sudo insmod pm_demo_device.ko
sudo insmod pm_demo_driver.ko
readlink /sys/bus/platform/devices/pm-demo/driver
cat /sys/bus/platform/devices/pm-demo/power/runtime_status
cat /sys/bus/platform/devices/pm-demo/power/control
cat /sys/bus/platform/devices/pm-demo/power/autosuspend_delay_ms
```

Fragen:

- Welcher `runtime_status` erscheint kurz nach `probe()` – und warum ist das
  Gerät so schnell wieder `suspended`, obwohl `autosuspend_delay_ms = 2000`
  konfiguriert ist? (Tipp: Der Driver-Core fordert nach erfolgreichem
  `probe()` einen Idle-Übergang an; ohne vorherigen `mark_last_busy()`
  gilt die Autosuspend-Verzögerung bereits als abgelaufen.)
- Was bedeutet `power/control = auto` für die Autosuspend-Policy?

### Aufgabe 2: Balancierten Zugriff auslösen

```bash
echo 1 | sudo tee /sys/bus/platform/devices/pm-demo/access
cat /sys/bus/platform/devices/pm-demo/access_count
sudo dmesg | tail -n 10
```

Fragen:

- Welche Log-Zeile zeigt den Get-Pfad, welche den Put-Pfad?
- Warum bleibt das Gerät nach dem Zugriff noch aktiv?

### Aufgabe 3: Autosuspend beobachten

```bash
sleep 3
cat /sys/bus/platform/devices/pm-demo/power/runtime_status
sudo dmesg | tail -n 10
```

Fragen:

- Nach welcher Zeit schaltet der Autosuspend das Gerät ab?
- Welche Ressource simuliert `runtime_suspend()` als "abgeschaltet"?
- Was passiert mit `shadow_reg`, wenn das Gerät wieder aufgeweckt wird?

### Aufgabe 4: Erneuter Zugriff nach Suspend

```bash
echo 1 | sudo tee /sys/bus/platform/devices/pm-demo/access
cat /sys/bus/platform/devices/pm-demo/power/runtime_status
sudo dmesg | tail -n 10
```

Fragen:

- Welcher Callback läuft vor dem eigentlichen Zugriff?
- Woran erkennst du in `dmesg`, dass `shadow_reg` neu restauriert wurde?

### Aufgabe 5: Wakeup simulieren

```bash
cat /sys/bus/platform/devices/pm-demo/power/wakeup
echo 1 | sudo tee /sys/bus/platform/devices/pm-demo/simulate_wakeup
cat /sys/bus/platform/devices/pm-demo/power/wakeup_count 2>/dev/null || true
sudo dmesg | tail -n 5
```

Fragen:

- Was zeigt `power/wakeup`, bevor überhaupt ein Ereignis ausgelöst wurde?
- Warum verhindert ein Wakeup-Event einen konkurrierenden Suspend?

### Aufgabe 6: Rückbau

```bash
sudo rmmod pm_demo_driver
sudo rmmod pm_demo_device
```

Fragen:

- Welche Aufräumschritte laufen in `remove()`, bevor `pm_runtime_disable()`
  greift?
- Warum muss `device_init_wakeup(dev, false)` vor `pm_runtime_disable()`
  stehen?

## Erwartete Ergebnisse

Typischer Verlauf:

1. Nach `probe()`: `power/control = auto`, `autosuspend_delay_ms = 2000`,
   `runtime_status` kippt aber innerhalb von Millisekunden auf `suspended` –
   der Driver-Core fordert nach erfolgreichem `probe()` einen Idle-Übergang
   an, und ohne vorherigen `mark_last_busy()` gilt die Autosuspend-Zeit
   bereits als abgelaufen.
2. Nach einem `access`-Zugriff: Log zeigt zuerst `runtime_resume` (Get holt
   das Gerät aus `suspended`), dann den Zugriff selbst; Gerät ist danach
   `active`.
3. Nach Ablauf der Autosuspend-Verzögerung (Zeitpunkt des Get, nicht des
   Probes): `runtime_status = suspended`, `dmesg` zeigt `runtime_suspend`.
4. Ein erneuter Zugriff auf ein suspended Gerät löst zuerst
   `runtime_resume()` aus und restauriert `shadow_reg`.
5. `simulate_wakeup` erzeugt ein Wakeup-Event, sichtbar in `dmesg` und in
   den `power/wakeup*`-Zählern.
6. `rmmod` entfernt Treiber und Gerät ohne Fehler; `remove()` deaktiviert
   Wakeup vor Runtime-PM.

## Musterlösung

- Jeder Zugriff ist durch `pm_runtime_resume_and_get()` /
  `pm_runtime_put_autosuspend()` symmetrisch geklammert – das ist die
  balancierte PM-Referenz aus Folie 07.
- `runtime_suspend()`/`runtime_resume()` behandeln Ressourcen in fester,
  zueinander gespiegelter Reihenfolge (Folie 05/08).
- `.suspend`/`.resume` auf `pm_runtime_force_suspend`/`_resume` zu legen
  vermeidet doppelte Logik für System-Sleep (Folie 09).
- `device_init_wakeup()` und `pm_wakeup_dev_event()` entkoppeln, welche
  Ereignisquellen einen Suspend verhindern dürfen (Folie 10).
- sysfs (`power/runtime_status`, `power/control`,
  `power/autosuspend_delay_ms`, `power/wakeup*`) und `dmesg` liefern
  zusammen die Evidenz für einen korrekten Energieübergang (Folie 12/13).

## Hinweise

- Die Übung nutzt ein künstliches Platform-Gerät; es wird keine reale
  Hardware angesprochen.
- Es wird **kein** echter System-Suspend (`echo mem > /sys/power/state`)
  ausgelöst – das würde die gesamte Trainingsmaschine schlafen legen.
  `.suspend`/`.resume` sind zwar verdrahtet, werden hier aber nur über den
  Code besprochen, nicht ausgeführt.
- Read-only auditieren, wo möglich: `power/control` und
  `autosuspend_delay_ms` werden in dieser Übung nicht umkonfiguriert.

## Optional: Automatisiertes Test-Script

```bash
sudo ./run.sh
```

Baut beide Module, lädt sie, prüft die Bindung, löst einen Zugriff aus,
wartet die Autosuspend-Verzögerung ab, zeigt den erneuten Resume-Pfad,
simuliert ein Wakeup-Event und entfernt beide Module wieder.

## Abschluss

Nach dieser Übung sollten die Teilnehmenden in einem Satz beschreiben
können, welche Referenz und welche Reihenfolge einen sicheren
Energieübergang beweisen – gestützt auf `dmesg` und die `power/*`-Dateien
aus dieser Übung.
