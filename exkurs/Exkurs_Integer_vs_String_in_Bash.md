# Exkurs: Integer vs. String im Terminal (Bash)

## Grundidee

In der Shell sind viele Werte zunächst einfach **Text**.

Ein Befehl wie:

```bash
echo 750
```

schreibt die Zeichen:

```text
7 5 0
```

auf die Standardausgabe.

Ob daraus später eine Zahl wird, entscheidet erst das Programm oder die Kernel-Schnittstelle, die diesen Text einliest.

## Beispiel: sysfs-Parameter schreiben

```bash
echo 750 | sudo tee /sys/module/demo/parameters/interval_ms
```

Was passiert hier?

```text
echo 750
   |
   | schreibt Text "750\n"
   v
Pipe
   |
   v
sudo tee ...
   |
   | schreibt Text in sysfs-Datei
   v
Kernel
   |
   | interpretiert "750"
   v
Integer-Wert 750
```

Wichtig:

> `echo` übergibt **keinen C-Integer**, sondern eine Folge von Zeichen.

Der Kernel konvertiert diese Zeichen später in einen Integer.

## String vs. Integer

### String

Ein String ist eine Zeichenfolge:

```text
"750"
```

Intern betrachtet:

```text
'7' '5' '0'
```

### Integer

Ein Integer ist ein numerischer Wert:

```text
750
```

Er kann für Berechnungen verwendet werden:

```text
750 + 250 = 1000
```

Die Darstellung `"750"` und der Zahlenwert `750` sind also nicht dasselbe.

## Warum funktioniert `echo 750` trotzdem?

Weil viele Linux-Schnittstellen textbasiert sind.

Beispiele:

```text
/proc
/sys
Shell-Umgebungsvariablen
Konfigurationsdateien
```

Das Programm oder der Kernel bekommt Text und wandelt ihn anschließend um.

```text
Userspace:
"750"
   |
   v
Kernel:
parse("750")
   |
   v
Integer:
750
```

## Beispiel mit Kernelmodul-Parameter

Angenommen ein Kernelmodul definiert:

```c
static int interval_ms = 1000;
module_param(interval_ms, int, 0644);
```

Dann erzeugt Linux beispielsweise:

```text
/sys/module/demo/parameters/interval_ms
```

Beim Schreiben:

```bash
echo 750 | sudo tee /sys/module/demo/parameters/interval_ms
```

kommt im Kernel sinngemäß an:

```text
"750"
```

Der Parametermechanismus erkennt den Typ:

```text
int
```

und konvertiert:

```text
"750" -> 750
```

## Was passiert bei ungültigem Text?

Beispiel:

```bash
echo hallo | sudo tee /sys/module/demo/parameters/interval_ms
```

Wenn der Parameter vom Typ `int` ist, kann `"hallo"` nicht sinnvoll in eine Zahl umgewandelt werden.

Typischerweise schlägt der Schreibvorgang dann fehl:

```text
Invalid argument
```

Der Kernel akzeptiert also nicht beliebigen Text, sondern prüft ihn gegen den erwarteten Datentyp.

## Bash-Variablen sind grundsätzlich textuell

Beispiel:

```bash
x=750
```

`x` ist zunächst eine Shell-Variable mit dem Inhalt:

```text
750
```

Bei Ausgabe:

```bash
echo "$x"
```

wird Text ausgegeben.

Für Arithmetik kann Bash den Inhalt numerisch interpretieren:

```bash
echo $((x + 250))
```

Ergebnis:

```text
1000
```

## Quotes ändern nicht den Datentyp

Diese beiden Befehle liefern praktisch denselben Text:

```bash
echo 750
```

und:

```bash
echo "750"
```

Die Quotes beeinflussen die Verarbeitung durch die Shell, nicht den späteren Datentyp beim Empfänger.

## Unterschied zu Programmiersprachen

In C ist der Typ explizit:

```c
int x = 750;
char *s = "750";
```

Das sind zwei unterschiedliche Objekte:

```text
x -> Integer 750
s -> String "750"
```

In Bash dagegen:

```bash
x=750
```

ist `x` zunächst textuell gespeichert und wird je nach Kontext numerisch interpretiert.

## Pipes transportieren Bytes

Eine Pipe:

```bash
echo 750 | tee datei
```

transportiert keine C-Datentypen.

Sie transportiert lediglich Bytes:

```text
'7' '5' '0' '\n'
```

Erst der Empfänger entscheidet, wie diese Bytes zu interpretieren sind.

Das gilt auch für:

```text
stdin
stdout
Pipes
Dateien
/proc
/sys
```

## Besonderheit bei `echo`

`echo` hängt normalerweise einen Zeilenumbruch an:

```bash
echo 750
```

liefert:

```text
750\n
```

Viele sysfs-Schnittstellen akzeptieren diesen Zeilenumbruch problemlos.

Alternativ:

```bash
printf '%s\n' 750
```

`printf` ist oft besser kontrollierbar als `echo`.

## Warum `sudo tee` statt `sudo echo ... > Datei`?

Dieser Befehl funktioniert oft nicht:

```bash
sudo echo 750 > /sys/module/demo/parameters/interval_ms
```

Grund:

```text
sudo echo ...
```

läuft mit Root-Rechten,

aber:

```text
>
```

wird von der aktuellen Shell ausgeführt.

Die Shell besitzt eventuell keine Schreibrechte.

Darum funktioniert:

```bash
echo 750 | sudo tee /sys/module/demo/parameters/interval_ms
```

Hier läuft der eigentliche Schreibvorgang in `tee` mit Root-Rechten.

## Gesamtbild

```text
Bash
 |
 | echo 750
 v
Text: "750\n"
 |
 v
Pipe
 |
 v
sudo tee
 |
 v
sysfs
 |
 v
Kernel-Parameter-Parser
 |
 v
Integer: 750
```

## Merksatz

> **Im Terminal werden Werte meist als Text übertragen. Ob `"750"` als Integer 750 interpretiert wird, entscheidet erst der Empfänger – zum Beispiel Bash bei Arithmetik oder der Linux-Kernel beim Parsen eines sysfs-Parameters.**
