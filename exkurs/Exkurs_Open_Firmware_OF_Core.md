# Exkurs: Open Firmware (OF) und Device Tree im Linux-Kernel

**OF (Open Firmware)** bezeichnet im Linux-Kernel die Infrastruktur zum Einlesen und Auswerten des **Device Trees**. Dieser beschreibt Hardware unabhängig vom Treibercode. Der OF Core liegt überwiegend unter `drivers/of/` und stellt APIs für andere Kernel-Subsysteme bereit.

## Vom Device Tree zum Treiber

```text
DTS / DTSI --dtc--> DTB --Bootloader--> Linux
                                         |
                                         v
                                     OF Core
                                  struct device_node
                                         |
                               Geräte und Properties
                                         |
                                  Platform Bus
                                         |
                          compatible-Match → probe()
```

Ein Device Tree besteht aus Knoten und Eigenschaften (*Properties*). Der kompilierte **DTB** wird typischerweise vom Bootloader an den Kernel übergeben. Der OF Core verwaltet die Knoten als `struct device_node`. Bus-Subsysteme können anhand geeigneter Knoten Geräte erzeugen und mit Treibern verbinden. **Nicht jeder Device-Tree-Knoten wird automatisch zu einem Platform Device**: beispielsweise werden I²C- und SPI-Kinder über ihre jeweiligen Busse registriert.

## Beispiel: Hardwarebeschreibung (DTS)

```dts
demo@10000000 {
    compatible = "example,demo";
    reg = <0x10000000 0x1000>;
    status = "okay";
};
```

`compatible` identifiziert die Hardware für das Treiber-Matching, `reg` beschreibt die Registerressource im Adressformat des Elternknotens und `status = "okay"` aktiviert den Knoten. In realen Device Trees gelten außerdem die jeweiligen **Devicetree-Bindings** (z. B. Adress- und Zellformate).

## Beispiel: OF-Matching im Platform-Treiber

```c
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

static int demo_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    u32 interval;

    if (!np)
        return -ENODEV;

    if (of_property_read_u32(np, "interval-ms", &interval))
        interval = 1000;  /* Vorgabe, falls Property fehlt */

    dev_info(&pdev->dev, "Intervall: %u ms\n", interval);
    return 0;
}

static const struct of_device_id demo_of_match[] = {
    { .compatible = "example,demo" },
    { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);

static struct platform_driver demo_driver = {
    .probe = demo_probe,
    .driver = {
        .name = "demo",
        .of_match_table = demo_of_match,
    },
};
module_platform_driver(demo_driver);
MODULE_LICENSE("GPL");
```

`of_match_table` verbindet den Treiber mit passenden DT-Knoten. `MODULE_DEVICE_TABLE(of, ...)` exportiert passende Modul-Aliase für das automatische Laden; das eigentliche Geräte-Matching erledigt der jeweilige Bus. Die optionale Property `interval-ms` müsste in der DTS ergänzt werden, wenn ein vom Standard abweichender Wert verwendet werden soll.

## Nützliche OF-APIs

| API | Zweck |
|---|---|
| `of_find_node_by_path()` | Knoten per Pfad suchen; Rückgabe später mit `of_node_put()` freigeben |
| `of_property_read_u32()` | `u32`-Property auslesen |
| `of_property_read_string()` | String-Property auslesen |
| `of_device_is_available()` | Verfügbarkeit über `status` prüfen |
| `of_match_device()` | Gerät gegen eine OF-Match-Tabelle prüfen |

Den vom Gerät geliehenen Zeiger `dev.of_node` dagegen **nicht** einfach mit `of_node_put()` freigeben. Für Register, Interrupts, Clocks oder GPIOs sollten Treiber nach Möglichkeit die entsprechenden verwalteten Subsystem-APIs (z. B. `devm_platform_ioremap_resource()`) verwenden, statt die DT-Eigenschaften selbst zu interpretieren.

## Diagnose und Abgrenzung

```bash
ls /sys/firmware/devicetree/base/
find /sys/firmware/devicetree/base/ -name compatible
# Falls dtc installiert ist und die Live-DT-Schnittstelle vorhanden ist:
dtc -I fs -O dts /sys/firmware/devicetree/base/ > running.dts
```

**OF/Device Tree** beschreibt Hardware deklarativ, insbesondere auf vielen ARM- und Embedded-Systemen. **ACPI** erfüllt auf vielen PCs eine verwandte Aufgabe, verwendet aber andere Tabellen, Methoden und Kernel-Schnittstellen. Moderne Treiber können teils beide Firmware-Beschreibungen unterstützen.

> **Merksatz:** Der OF Core ist die Device-Tree-Infrastruktur des Kernels: Er stellt Hardwareknoten und Eigenschaften bereit, während Bus-Subsysteme daraus Geräte erzeugen und über `compatible` passende Treiber zuordnen.
