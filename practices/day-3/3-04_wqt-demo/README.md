# wqt_demo - Timer treibt eine Workqueue

Trainingsbeispiel für Tag 3: ein periodischer Timer plant per
`schedule_work()` Arbeit ein, die im Prozesskontext eines Workers läuft.
Timer-Callback und Worker sind bewusst getrennt:

- **Timer** (`demo_timer_fn`): Softirq-Kontext, bleibt kurz - markiert,
  queued die Work, rearmt sich selbst. Kein `msleep`, kein Mutex.
- **Worker** (`demo_work_fn`): Prozesskontext, darf blockieren
  (`msleep(work_ms)` simuliert Arbeit), zählt Ausführungen (`runs`).

Kann der Timer die Work nicht neu einplanen, weil sie noch aussteht, wird
`coalesced` erhöht - sichtbar, wenn `work_ms` größer als `period_ms` ist.

## Bauen

```bash
make
```

## Laden und Beobachten

```bash
sudo ./run.sh [PERIOD_MS] [WORK_MS] [BEOBACHTUNGSDAUER_S]
```

Baut das Modul, lädt es mit den angegebenen Parametern, zeigt für die
angegebene Dauer die `dmesg`-Ausgabe (`run=N cpu=X`) und entlädt es wieder
(auch bei Abbruch via Trap).

Beispiel Normalbetrieb: `sudo ./run.sh 500 100 5`
Beispiel Überlast/Koaleszierung: `sudo ./run.sh 100 350 5`

## Laufzeitparameter

```bash
cat /sys/module/wqt_demo/parameters/period_ms
cat /sys/module/wqt_demo/parameters/work_ms
```

Beide Parameter sind `0644` und lassen sich zur Laufzeit per `echo ... | sudo tee ...` ändern.

## Manuelles Entladen

```bash
sudo rmmod wqt_demo
```

Beim Entladen wird zuerst der Timer gestoppt (`timer_shutdown_sync`), dann
auf die laufende Work gewartet (`cancel_work_sync`), bevor die Endstatistik
(`runs`, `coalesced`) geloggt wird.
