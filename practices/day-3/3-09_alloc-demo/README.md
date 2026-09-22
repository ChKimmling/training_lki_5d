# alloc_demo - Dynamische Speicherallokation mit vollständigen Fehlerpfaden

Trainingsbeispiel für Tag 3: zeigt sauberes Ownership und Rückrollen bei
mehrstufiger Kernel-Allokation.

## Allokationsebenen

| Ebene | Ressource | Erzeuger | Owner | Freigabe |
|---|---|---|---|---|
| 1 | `items[]` (Pointer-Array) | `demo_init()` | Modul | `kfree(items)` |
| 2 | `demo_item` | `item_create()` | `items[i]` | `kfree(item)` |
| 3 | `payload` | `item_create()` | `demo_item` | `kfree(item->payload)` |

Regel: Nur der festgelegte Owner gibt eine Ressource frei - genau einmal.
`item_create()` veröffentlicht ausschließlich vollständig erzeugte Objekte;
schlägt die Payload-Allokation fehl, wird das Objekt sofort wieder verworfen.

`create_items()` reserviert Objekte 0..count-1 der Reihe nach. Schlägt eine
Allokation fehl, rollt die Schleife rückwärts von `i` bis 0 zurück und gibt
danach das Pointer-Array frei - der Index beschreibt jederzeit den
Teilerfolg.

## Modulparameter

- `count` (Default 4) - Anzahl der Objekte, `1..1024`
- `payload_size` (Default 128) - Payload-Größe je Objekt in Bytes, `1..PAGE_SIZE`
- `fail_after` (Default -1) - simuliert deterministisch einen Allokationsfehler
  ab diesem Index, um den Rückrollpfad reproduzierbar zu testen

Ungültige `count`/`payload_size`-Werte werden vor jeder Allokation mit
`-EINVAL` abgelehnt.

## Bauen

```bash
make
```

## Testen

```bash
sudo ./run.sh
```

Führt die Testmatrix aus dem Training automatisiert durch:

1. Normalfall (`count=4 payload_size=128`)
2. `count=0` / `payload_size=0` → `-EINVAL`, keine Allokation begonnen
3. `fail_after=3` bei `count=8` → `-ENOMEM`, Objekte 0-2 werden zurückgerollt
4. 50× Laden/Entladen zur Stabilitätsprobe

## Manuell

```bash
sudo insmod alloc_demo.ko count=8 payload_size=256
sudo dmesg | tail -n 20
sudo rmmod alloc_demo
sudo dmesg | tail -n 20
```
