#!/usr/bin/env bash
#
# test_data_race.sh - Provoziert den Data Race in data_race.ko: zwei
# konkurrierende Worker schreiben je Iteration gleichzeitig auf
# /dev/data-race (jeder Schreibzugriff öffnet das Gerät und ruft damit
# reserve_slot() auf).
#
# Aufruf: sudo ./test_data_race.sh [ITERATIONEN] [DELAY_US]

set -euo pipefail

DEVICE=/dev/data-race
MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODULE=data_race.ko
ITERATIONS="${1:-1000}"
DELAY_US="${2:-0}"

if [[ $EUID -ne 0 ]]; then
	echo "Bitte als root ausführen (insmod/rmmod erfordern Rechte)." >&2
	exit 1
fi

if lsmod | grep -q '^data_race'; then
	echo "Modul bereits geladen, entferne es zuerst..."
	rmmod data_race
fi

echo "Lade Modul mit delay_us=${DELAY_US} ..."
insmod "${MODULE_DIR}/${MODULE}" delay_us="${DELAY_US}"

cleanup() {
	echo "Entferne Modul..."
	rmmod data_race || true
}
trap cleanup EXIT

if [[ ! -c "${DEVICE}" ]]; then
	echo "Fehler: ${DEVICE} existiert nicht." >&2
	exit 1
fi

echo "Starte ${ITERATIONS} Iterationen mit je zwei konkurrierenden Workern ..."

# Zwei konkurrierende Worker
for i in $(seq 1 "${ITERATIONS}"); do
	echo reserve > "${DEVICE}" &
	echo reserve > "${DEVICE}" &
	wait
done

dmesg | tail -n 50
