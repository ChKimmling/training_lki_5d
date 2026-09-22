# labchar - Einfacher Character-Device-Treiber

Trainingsbeispiel für Tag 3: minimaler Character-Device-Treiber mit
gepuffertem `read`/`write`, konsistentem Kernel-Snapshot fürs Locking und
symmetrischem Erwerb/Rückbau der Kernel-Ressourcen (`dev_t`, `cdev`,
`class`, `device`).

## Aufbau

- **`lab_open`/`lab_release`** – lösen die Geräteinstanz per `container_of`
  auf und legen sie in `file->private_data` ab.
- **`lab_read`** – kopiert den Puffer unter dem Mutex in einen kurzen
  Kernel-Snapshot, entsperrt und übergibt ihn erst danach per
  `simple_read_from_buffer` an den Userspace.
- **`lab_write`** – akzeptiert maximal 127 Nutzbytes (`LAB_SIZE - 1`),
  übernimmt Daten nur nach erfolgreichem `copy_from_user` und committet den
  gemeinsamen Zustand unter dem Mutex. Bei zu großer Eingabe liefert der
  Rückgabewert bewusst einen Partial Write.
- **`lab_init`/`lab_exit`** – erwerben `dev_t → cdev → class → device` und
  rollen im Fehlerfall bzw. beim Entladen in exakt umgekehrter Reihenfolge
  zurück.

## Bauen

```bash
make
```

## Testen

```bash
sudo ./run.sh
```

Baut das Modul, lädt es, prüft die Registrierung (`lsmod`,
`/proc/devices`, `/dev/labchar0`) und führt die Funktionstests aus dem
Training aus:

1. Happy Path: schreiben → lesen liefert denselben Wert
2. Overwrite: ein zweiter Write ersetzt den vorherigen Zustand
3. Partial Write: ein einzelner `write()`-Aufruf mit 200 Bytes liefert den
   Rückgabewert 127
4. Rückbau: `rmmod` entfernt `/dev/labchar0` wieder

## Manuell

```bash
sudo insmod labchar.ko
printf 'hello' | sudo tee /dev/labchar0 >/dev/null
sudo cat /dev/labchar0
sudo rmmod labchar
```
