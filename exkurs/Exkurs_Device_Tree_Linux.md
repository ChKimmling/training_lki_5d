# Exkurs: Device Tree im Linux-Kernel

**Device Tree (DT)** beschreibt die Hardware eines Systems unabhängig vom Treibercode. Er wird besonders auf Embedded-Plattformen wie ARM und RISC-V genutzt, auf denen Geräte nicht immer automatisch erkannt werden können. Der Bootloader übergibt dem Kernel in der Regel einen kompilierten **Device Tree Blob (DTB)**.

## 1. Vom Quelltext zum Treiber

```text
DTS / DTSI (Hardwarebeschreibung)
          |  Device Tree Compiler (dtc)
          v
         DTB
          |  Bootloader übergibt DTB
          v
      Linux-Kernel
          |  Firmware-/OF-Subsystem
          v
  Geräte anlegen und Treiber zuordnen
          |  compatible-Abgleich
          v
      probe()-Callback
```

- **`.dts`**: Board-spezifischer Device-Tree-Quelltext.
- **`.dtsi`**: Wiederverwendete SoC- oder Plattformbeschreibung, per `#include` eingebunden.
- **`.dtb`**: Binäre Darstellung für Bootloader und Kernel.
- **Device Tree Overlay (`.dtbo`)**: Ergänzt oder verändert einen bestehenden Device Tree, sofern Plattform und Bootablauf dies unterstützen.

## 2. Beispiel eines Geräteknotens

```dts
/ {
    demo@40000000 {
        compatible = "example,demo-v1";
        reg = <0x40000000 0x1000>;
        interrupts = <42>;
        status = "okay";
    };
};
```

**Achtung:** Die Kodierung von `reg` hängt von `#address-cells` und `#size-cells` des Elternknotens ab; `interrupts` wird durch den jeweiligen Interrupt-Controller und seine `#interrupt-cells` definiert. Die Werte oben dienen nur der Veranschaulichung.

| Eigenschaft | Bedeutung |
|---|---|
| `compatible` | Kennung für die Zuordnung eines passenden Treibers |
| `reg` | Adressbereiche und Größen der Hardware-Ressourcen |
| `interrupts` | Interrupt-Spezifikation gemäß Interrupt-Controller |
| `status` | `"okay"`: Gerät aktiviert; `"disabled"`: nicht verfügbar |
| `clocks`, `resets`, `*-gpios` | Referenzen auf weitere Hardware-Ressourcen |

## 3. Zuordnung zum Platform-Treiber

```c
static const struct of_device_id demo_of_match[] = {
    { .compatible = "example,demo-v1" },
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
```

Bei passendem `compatible` kann der Kernel ein Platform-Device mit dem Treiber verbinden und dessen `probe()` aufrufen. Dort fordert der Treiber beispielsweise MMIO-Bereiche, IRQs oder Clocks über die entsprechenden Kernel-APIs an. **Der Device Tree beschreibt Hardware, nicht den Ablauf des Treibers.**

## 4. Praktische Analyse

```bash
# Vom Kernel erkannte Device-Tree-Knoten (falls vorhanden)
ls /sys/firmware/devicetree/base/

# Laufenden Device Tree als DTS dekompilieren (dtc erforderlich)
dtc -I fs -O dts /proc/device-tree > running.dts

# Device-Tree-Dateien im Kernel-Quellbaum finden
find arch/ -path '*/boot/dts/*' -name '*.dts' | head
```

`/proc/device-tree` ist auf vielen Systemen ein Verweis auf `/sys/firmware/devicetree/base`. Auf typischen ACPI-basierten PCs kann dieser Pfad fehlen. Für produktive Hardwarebeschreibungen gelten die **Device-Tree-Bindings** unter `Documentation/devicetree/bindings/` als maßgebliche Schema- und Dokumentationsquelle.

**Merksatz:** *Der Device Tree beschreibt, welche Hardware vorhanden ist und welche Ressourcen sie besitzt; über `compatible` findet Linux den passenden Treiber und bindet das Gerät ein.*
