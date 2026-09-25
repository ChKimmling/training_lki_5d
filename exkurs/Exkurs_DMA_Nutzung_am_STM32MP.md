# Exkurs: DMA-Nutzung am STM32MP – SPI ohne CPU-Datentransport

Bei einem SPI-Transfer kann die CPU jedes Datenwort selbst zwischen Arbeitsspeicher und SPI-Datenregister bewegen. Das ist für kurze Nachrichten effizient, belastet bei größeren Transfers aber CPU und Interruptpfad. Der STM32MP kann diese Bewegung an einen DMA-Controller auslagern. Der SPI-Treiber konfiguriert dann nur noch Transfer, Richtung und Abschlussbehandlung; die Nutzdaten fließen direkt zwischen Speicher und SPI-Peripherie.

## Der Datenpfad im Überblick

```text
SPI-Gerätetreiber, z. B. TPM, ADC oder eigener Treiber
                │  spi_sync() / spi_async()
                ▼
             SPI-Core
                │  Scatter-Gather-Liste und transfer_one()
                ▼
        STM32-SPI-Controller-Treiber
          │         │             │
       Polling   Interrupt       DMA
                                  │  dmaengine_prep_slave_sg()
                                  ▼
                              DMAMUX1
                     ordnet SPI-Request einem
                       freien DMA-Stream zu
                                  │
                         ┌────────┴────────┐
                         ▼                 ▼
                       DMA1              DMA2
                         │                 │
                         └───────┬─────────┘
                                 ▼
                    SPI TXDR/RXDR ↔ Arbeitsspeicher
```

Der **DMAMUX** ist dabei kein eigener Datentransporteur. Er verbindet einen Peripherie-Request, beispielsweise SPI5-RX, mit einem freien Stream von DMA1 oder DMA2. Erst der ausgewählte DMA-Controller bewegt die Daten. RX und TX benötigen getrennte Requests und bei Vollduplexbetrieb in der Regel zwei DMA-Streams.

## Device Tree: SPI5 an DMAMUX und DMA anbinden

Beim STM32MP157 beschreibt `arch/arm/boot/dts/st/stm32mp151.dtsi` SPI5 einschließlich seiner DMA-Requests:

```dts
spi5: spi@44009000 {
	compatible = "st,stm32h7-spi";
	reg = <0x44009000 0x400>;
	interrupts = <GIC_SPI 85 IRQ_TYPE_LEVEL_HIGH>;
	clocks = <&rcc SPI5_K>;
	resets = <&rcc SPI5_R>;

	dmas = <&dmamux1 85 0x400 0x05>,
	       <&dmamux1 86 0x400 0x05>;
	dma-names = "rx", "tx";
	status = "disabled";
};
```

Die Reihenfolge ist wesentlich: Der erste Eintrag gehört zu `rx`, der zweite zu `tx`. `&dmamux1` verweist auf den DMA-Router; 85 und 86 sind die Peripherie-Request-Nummern. Die folgenden Zellen kodieren die STM32-DMA-Konfiguration gemäß dem zur Kernelversion gehörenden Binding. Solche Werte sollten nicht von einer anderen SoC- oder Kernelversion übernommen werden.

Der zugehörige Router verbindet seine 16 Kanäle mit DMA1 und DMA2:

```dts
dmamux1: dma-router@48002000 {
	compatible = "st,stm32h7-dmamux";
	#dma-cells = <3>;
	dma-requests = <128>;
	dma-masters = <&dma1 &dma2>;
	dma-channels = <16>;
};
```

Eine Board-DTS aktiviert anschließend den Controller, wählt die Pins und beschreibt das angeschlossene SPI-Gerät. Die DMA-Zuordnung muss dort normalerweise nicht wiederholt werden:

```dts
&spi5 {
	pinctrl-names = "default";
	pinctrl-0 = <&spi5_pins_a>;
	cs-gpios = <&gpiof 6 GPIO_ACTIVE_LOW>;
	status = "okay";

	peripheral@0 {
		compatible = "hersteller,geraet";
		reg = <0>;
		spi-max-frequency = <10000000>;
	};
};
```

## Vom Probe bis zum DMA-Abschluss

Der Controller-Knoten bindet über `compatible = "st,stm32h7-spi"` an `drivers/spi/spi-stm32.c`. In `stm32_spi_probe()` fordert der Treiber die benannten Kanäle an:

```c
spi->dma_tx = dma_request_chan(spi->dev, "tx");
spi->dma_rx = dma_request_chan(spi->dev, "rx");

if (spi->dma_tx || spi->dma_rx)
	ctrl->can_dma = stm32_spi_can_dma;
```

`dma_request_chan()` folgt `dma-names` und `dmas` im Device Tree. Der OF-DMA-Code erreicht zunächst `stm32-dmamux.c`; der Router reserviert einen freien Ausgang, programmiert dessen Request-Nummer und reicht eine angepasste Kanalspezifikation an `stm32-dma.c` weiter.

Für jeden `spi_transfer` entscheidet `stm32_spi_transfer_one()` neu zwischen DMA, Polling und Interrupt. Auf STM32-Varianten mit FIFO wird DMA verwendet, wenn der Transfer größer als der FIFO ist. Kleine Nachrichten bleiben bewusst im PIO-Pfad, weil Aufbau, Mapping und Abbau eines DMA-Transfers ebenfalls Zeit kosten.

Im DMA-Pfad konfiguriert `stm32_spi_transfer_one_dma()` die Richtung und die Adresse des SPI-Datenregisters. Danach erzeugt `dmaengine_prep_slave_sg()` Deskriptoren aus den vom SPI-Core vorbereiteten Scatter-Gather-Listen. `dmaengine_submit()` stellt sie ein, `dma_async_issue_pending()` startet die Kanäle und der SPI-Controller aktiviert seine RX-/TX-DMA-Requests. Nach dem DMA-Callback beziehungsweise dem SPI-Ende signalisiert der Treiber dem SPI-Core den Transferabschluss.

## Warum Scatter-Gather wichtig ist

Ein logischer SPI-Puffer muss physisch nicht zusammenhängend sein. Der SPI-Core mappt den Puffer deshalb für das DMA-Gerät und stellt eine Scatter-Gather-Liste bereit. Der DMA-Treiber arbeitet die Segmente ab, ohne dass der SPI-Controller-Treiber sie in einen großen zusammenhängenden Zwischenpuffer kopieren muss.

Treiber dürfen dabei nicht mit normalen CPU-Adressen an der DMA-Hardware arbeiten. Mapping, Cache-Kohärenz, Richtung und Lebensdauer der Abbildung werden über DMA-API und SPI-Core verwaltet. Ein Buffer darf während des laufenden Transfers weder freigegeben noch unkontrolliert verändert werden.

## Fehlerverhalten und Rückfall auf PIO

Fehlen `dmas` oder ein benannter Kanal, meldet der STM32-SPI-Treiber sinngemäß `tx dma disabled` beziehungsweise `rx dma disabled`. SPI kann trotzdem über Polling oder Interrupt funktionieren. Das ist nützlich für Robustheit, kann aber eine fehlerhafte DTS-Konfiguration verdecken: Ein funktionierendes SPI-Gerät beweist noch nicht, dass DMA aktiv ist.

Typische Fehlerquellen sind vertauschte `dma-names`, falsche Request-Nummern, ein nicht geladener DMA-/DMAMUX-Treiber, Pinmux-Konflikte, nicht DMA-fähige Puffer sowie Transfers, die unterhalb der DMA-Schwelle bleiben. Bei Vollduplex müssen RX und TX gemeinsam verfügbar sein; ein nur teilweise vorbereiteter Transfer wird abgebrochen.

## DMA-Nutzung beobachten

```bash
zgrep -E 'SPI_STM32|STM32_DMA|STM32_DMAMUX' /proc/config.gz
dmesg | grep -Ei 'spi_stm32|dma|dmamux'
ls -l /sys/bus/platform/devices/44009000.spi
ls -l /sys/class/spi_master /sys/bus/spi/devices

# optional, falls DebugFS und DMAengine-Debug verfügbar sind
find /sys/kernel/debug/dmaengine -maxdepth 3 -type f -print
```

Für einen belastbaren Nachweis eignen sich dynamische Debugmeldungen oder Funktions-Tracing an `stm32_spi_can_dma()`, `stm32_spi_transfer_one_dma()` und den DMA-Callbacks. Testnachrichten müssen größer als die FIFO-basierte DMA-Schwelle sein. Ein Logikanalysator zeigt den Transfer auf den Leitungen, unterscheidet aber allein nicht zwischen PIO und DMA.

## Merksätze

- Der SPI-Gerätetreiber fordert SPI-Transfers an; der STM32-SPI-Controller-Treiber entscheidet über DMA.
- Der DMAMUX routet Requests, während DMA1 oder DMA2 die Daten tatsächlich bewegt.
- RX und TX sind getrennte DMA-Kanäle; Vollduplex benötigt beide Richtungen.
- DMA lohnt sich vor allem für größere Transfers. Kleine Transfers bleiben absichtlich im Polling- oder Interruptpfad.
- Funktionierendes SPI bedeutet nicht automatisch aktives DMA; DTS, Kernelkonfiguration und Laufzeitpfad müssen getrennt geprüft werden.

## Quellen und Vertiefung

- [Linux-Quellcode: STM32-SPI-Treiber](https://github.com/torvalds/linux/blob/master/drivers/spi/spi-stm32.c)
- [Device-Tree-Binding: STM32-SPI](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/spi/st%2Cstm32-spi.yaml)
- [STM32MP151 Device Tree mit SPI, DMA und DMAMUX](https://github.com/torvalds/linux/blob/master/arch/arm/boot/dts/st/stm32mp151.dtsi)
- [Linux-Quellcode: STM32-DMA](https://github.com/torvalds/linux/blob/master/drivers/dma/stm32/stm32-dma.c)
- [Linux-Quellcode: STM32-DMAMUX](https://github.com/torvalds/linux/blob/master/drivers/dma/stm32/stm32-dmamux.c)
- [Linux-Kernel-Dokumentation: DMAengine Client API](https://docs.kernel.org/driver-api/dmaengine/client.html)
- [Linux-Kernel-Dokumentation: DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)
