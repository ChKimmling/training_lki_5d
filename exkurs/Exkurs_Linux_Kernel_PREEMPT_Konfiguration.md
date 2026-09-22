# Exkurs: PREEMPT-Konfiguration des Linux-Kernels

**Preemption** bedeutet, dass der Scheduler einen laufenden Task unterbrechen und einen anderen ausführen kann. Die Kernel-Konfiguration beeinflusst dabei vor allem, **wann auch Kernel-Code präemptierbar ist**. Mehr Präemptierbarkeit verkürzt oft Reaktionszeiten, erhöht aber unter Umständen den Verwaltungsaufwand.

## 1. Präemptionsmodelle

| Kconfig-Option | Bedeutung |
|---|---|
| `CONFIG_PREEMPT_NONE` | Keine erzwungene Kernel-Präemption; Schwerpunkt Durchsatz. Auf aktuellen Architekturen ggf. nicht auswählbar. |
| `CONFIG_PREEMPT_VOLUNTARY` | Zusätzliche freiwillige Scheduling-Punkte; Kompromiss zwischen Durchsatz und Reaktionszeit. |
| `CONFIG_PREEMPT` | Voll präemptierbarer normaler Kernel, außer in ausdrücklich geschützten/atomaren Bereichen. |
| `CONFIG_PREEMPT_LAZY` | Scheduler-gesteuerte „träge“ Präemption: gewöhnliche Tasks werden weniger aggressiv unterbrochen; architekturabhängig. |
| `CONFIG_PREEMPT_RT` | Echtzeitvariante mit weitreichender Präemptierbarkeit, Threaded IRQs und RT-fähigen Sperren; nur bei unterstützter Architektur und entsprechenden Abhängigkeiten. |

Diese Optionen bilden **nicht fünf gleichzeitig aktive Modi**: Die ersten vier stehen im Kconfig-Auswahlblock; `PREEMPT_RT` ist zusätzlich mit besonderen Abhängigkeiten definiert und beeinflusst die verfügbaren Modi. Die konkrete Auswahl hängt von Kernelversion und Architektur ab.

## 2. Weitere `PREEMPT_*`-Konfigurationssymbole

In der aktuellen Hauptdatei `kernel/Kconfig.preempt` finden sich neben den Modellen folgende Symbole:

| Kconfig-Option | Funktion |
|---|---|
| `CONFIG_PREEMPT_DYNAMIC` | Präemptionsverhalten eines entsprechend gebauten Kernels zur Boot-/Laufzeit auswählen. |
| `CONFIG_PREEMPT_NONE_BUILD` | Internes Build-Symbol für den nichtpräemptiven Modus. |
| `CONFIG_PREEMPT_VOLUNTARY_BUILD` | Internes Build-Symbol für freiwillige Präemption. |
| `CONFIG_PREEMPT_BUILD` | Internes Build-Symbol für präemptierbare Kernelvarianten. |
| `CONFIG_PREEMPTION` | Interne Kennzeichnung, dass Kernel-Präemption unterstützt/eingebaut ist. |
| `CONFIG_PREEMPT_COUNT` | Aktiviert die für Präemptions-/Kontextzustände verwendete Zählung. |
| `CONFIG_PREEMPT_RT_NEEDS_BH_LOCK` | Spezielle Testoption für SoftIRQ-Synchronisation unter PREEMPT_RT, kein regulärer Standardmodus. |

**Verwandte Architektursymbole** (ohne `PREEMPT_`-Präfix) sind unter anderem `ARCH_HAS_PREEMPT_LAZY`, `ARCH_NO_PREEMPT`, `ARCH_SUPPORTS_RT` und `HAVE_PREEMPT_DYNAMIC`. Andere Kernel-Unterverzeichnisse können weitere versions- oder architekturspezifische Optionen mit `PREEMPT_` im Namen definieren: Die obige Liste umfasst **alle entsprechenden Symbole der Hauptdatei `kernel/Kconfig.preempt`**, nicht pauschal jedes Symbol im gesamten Source Tree.

## 3. `PREEMPT_DYNAMIC` praktisch

Wenn `CONFIG_PREEMPT_DYNAMIC=y`, kann der Bootparameter das Modell auswählen, **soweit die Architektur und der jeweilige Build es unterstützen**:

```text
preempt=none
preempt=voluntary
preempt=full
preempt=lazy
```

Nicht jeder Modus ist auf jedem aktuellen Kernel verfügbar; unter anderem können `none`/`voluntary` auf Architekturen mit Lazy-Präemption entfallen. `preempt=full` ist **nicht gleichbedeutend mit** `CONFIG_PREEMPT_RT`.

## 4. Aktive Konfiguration untersuchen

```bash
# Konfiguration des laufenden Kernels (falls bereitgestellt)
zgrep -E '^CONFIG_(PREEMPT|ARCH_HAS_PREEMPT_LAZY)' /proc/config.gz

# Alternative auf vielen Distributionen
grep -E '^CONFIG_(PREEMPT|ARCH_HAS_PREEMPT_LAZY)' /boot/config-$(uname -r)

# Tatsächlich verwendeter Bootparameter
cat /proc/cmdline

# Nur falls vom Kernel angeboten: dynamischen Laufzeitmodus ansehen
cat /sys/kernel/debug/sched/preempt
```

Für eine **vollständige Liste der eigenen Kernelversion** im ausgecheckten Source Tree:

```bash
git grep -nE '^[[:space:]]*config PREEMPT_[A-Z0-9_]+' -- '*Kconfig*'
```

Beachte: `CONFIG_PREEMPT` in `.config` beschreibt bei `PREEMPT_DYNAMIC` nicht allein den **aktuell laufenden** Präemptionsmodus.

## Merksatz

> **`PREEMPT_*` bestimmt, wie schnell Linux einen laufenden Task auch innerhalb des Kernels unterbrechen darf: von durchsatzorientierter geringer Präemption über Full/Lazy bis zur RT-Variante mit umfangreichen Echtzeitanpassungen.**

## Quellen

- [Linux-Kernel: `kernel/Kconfig.preempt`](https://github.com/torvalds/linux/blob/master/kernel/Kconfig.preempt)
- [Linux-Kernel: Bootparameter `preempt=`](https://docs.kernel.org/admin-guide/kernel-parameters.html)
- [Linux-Kernel: PREEMPT_RT – Funktionsweise](https://docs.kernel.org/core-api/real-time/theory.html)
