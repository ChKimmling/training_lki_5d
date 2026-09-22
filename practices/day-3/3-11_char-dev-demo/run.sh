#!/usr/bin/env bash
#
# run.sh - Baut labchar.ko, lädt es, prüft die Registrierung (lsmod,
# /proc/devices, /dev/labchar0) und führt die Funktionstests aus Folie 14
# aus (Happy Path, Overwrite, Partial Write, Rückbau).
#
# Aufruf: sudo ./run.sh

set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODULE=labchar.ko
DEVICE=/dev/labchar0

if [[ $EUID -ne 0 ]]; then
	echo "Bitte als root ausführen (insmod/rmmod erfordern Rechte)." >&2
	exit 1
fi

echo "=== Build ==="
make -C "${MODULE_DIR}"
modinfo "${MODULE_DIR}/${MODULE}"

if lsmod | grep -q '^labchar'; then
	echo "Modul bereits geladen, entferne es zuerst..."
	rmmod labchar
fi

echo "=== insmod ==="
insmod "${MODULE_DIR}/${MODULE}"

cleanup() {
	if lsmod | grep -q '^labchar'; then
		rmmod labchar || true
	fi
}
trap cleanup EXIT

echo "=== Registrierung prüfen ==="
lsmod | grep labchar
grep labchar /proc/devices
ls -l "${DEVICE}"
dmesg | tail -n 20

echo
echo "=== Happy Path: schreiben -> lesen ==="
printf 'hello' | tee "${DEVICE}" >/dev/null
result="$(cat "${DEVICE}")"
echo "Gelesen: '${result}' (erwartet: 'hello')"

echo
echo "=== Overwrite ==="
sh -c "printf 'second value' > ${DEVICE}"
result="$(cat "${DEVICE}")"
echo "Gelesen: '${result}' (erwartet: 'second value')"

echo
echo "=== Partial Write (>127 Bytes, ein einzelner write()-Aufruf) ==="
python3 -c "
import os
fd = os.open('${DEVICE}', os.O_WRONLY)
n = os.write(fd, b'x' * 200)
os.close(fd)
print(f'write() Rueckgabewert: {n} (erwartet: 127)')
"
result="$(cat "${DEVICE}")"
echo "Länge gelesen: ${#result} (erwartet: 127)"

echo
echo "=== Rückbau ==="
trap - EXIT
rmmod labchar
if [[ ! -e "${DEVICE}" ]]; then
	echo "REMOVED: ${DEVICE} wurde entfernt"
else
	echo "Fehler: ${DEVICE} existiert noch" >&2
	exit 1
fi
