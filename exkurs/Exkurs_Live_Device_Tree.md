# Exkurs: Live Device Tree im Linux-Kernel

## Was ist der Live Device Tree?

Der **Live Device Tree** ist die zur Laufzeit vom Linux-**OF Core** verwaltete Baumstruktur der Hardwarebeschreibung. Beim Booten übergibt die Firmware bzw. der Bootloader meist einen kompakten **Flattened Device Tree (FDT/DTB)**. Linux prüft ihn und baut daraus eine navigierbare Hierarchie aus `struct device_node` und `struct property` auf.

```text
DTS + DTSI → dtc → DTB (FDT) → Bootloader → Linux/OF Core
                                             │
                                             ▼
                                      Live Device Tree
                                      struct device_node
                                             │
                               ┌─────────────┴─────────────┐
                               ▼                           ▼
                        Platform Devices              OF-Treiber-APIs
```

**Unterschied:** Das *DTB* ist die binäre, flache Übergabedarstellung; der *Live Device Tree* ist die vom Kernel zur Laufzeit verwaltete Objektstruktur. Er beschreibt Geräte – er ist weder ein Speicherabbild aller Gerätetreiber noch ein Beweis, dass jedes beschriebene Gerät tatsächlich gebunden ist.

## Beispiel und Treiberzugriff

```dts
sensor@48 {
    compatible = "vendor,demo-sensor";
    reg = <0x48>;
    status = "okay";
    sample-rate = <100>;
};
```

Ein passender Treiber kann seine OF-Match-Tabelle verwenden und Eigenschaften seines Knotens lesen:

```c
static const struct of_device_id demo_of_match[] = {
    { .compatible = "vendor,demo-sensor" },
    { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);

/* z. B. innerhalb der probe()-Funktion */
u32 rate;
int ret = of_property_read_u32(dev->of_node, "sample-rate", &rate);
if (ret)
    return ret;
```

Der Bus muss zum Gerät passen: Ein I²C-Knoten wird normalerweise unter einem I²C-Controller eingehängt und vom **I²C-Subsystem** angelegt, nicht pauschal als Platform Device.

## Live Device Tree untersuchen

Auf Device-Tree-Systemen lässt sich die laufende Beschreibung häufig unter `/sys/firmware/devicetree/base` einsehen; `/proc/device-tree` ist oft ein Verweis darauf:

```bash
ls /sys/firmware/devicetree/base
tr -d '\0' < /sys/firmware/devicetree/base/model
# Gesamten aktuell sichtbaren Baum als DTS ausgeben (dtc vorausgesetzt):
dtc -I fs -O dts /sys/firmware/devicetree/base > live.dts
```

**Achtung:** Properties liegen als Binärdaten vor: Strings sind nullterminiert, mehrzellige Zahlen werden üblicherweise **Big-Endian** codiert. Die rückgewonnene DTS-Datei entspricht nicht zwingend dem ursprünglichen Quelltext (z. B. fehlen Kommentare und ursprüngliche Includes/Labels).

## OF Core: Knoten suchen und Referenzen freigeben

```c
struct device_node *np;
u32 value;

np = of_find_node_by_path("/soc/demo@10000000");
if (!np)
    return -ENODEV;

ret = of_property_read_u32(np, "sample-rate", &value);
of_node_put(np);  /* Referenz der Suchfunktion freigeben */
```

Wichtige APIs: `of_find_node_by_path()`, `of_property_read_*()`, `of_device_is_available()`, `of_match_device()` und `of_node_put()`.

## Kann sich der Baum zur Laufzeit ändern?

**Ja, bei passender Kernelunterstützung:** Device-Tree-Overlays können Knoten und Properties dynamisch ergänzen bzw. verändern. Der OF-Core meldet Änderungen an beteiligte Subsysteme; ob daraus ein Gerät erzeugt oder ein Treiber gebunden wird, hängt vom jeweiligen Bus und Treiber ab. Overlays dürfen nicht beliebig entfernt werden, solange andere Komponenten deren Knoten oder Daten noch benötigen.

**Merksatz:** **Der Live Device Tree ist die vom OF Core verwaltete Laufzeitdarstellung des DTB: Kernel-Subsysteme und Treiber finden darin Hardwareknoten und lesen deren Eigenschaften.**
