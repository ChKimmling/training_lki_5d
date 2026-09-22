#!/usr/bin/env bash
#
# run.sh - Baut wqt_demo.ko, lädt es, zeigt kurz das Kernel-Log und
# entlädt es wieder. Optional kann ein Lastwechsel (Überlast) simuliert
# werden, um Koaleszierung zu provozieren.
#
# Aufruf: sudo ./run.sh [PERIOD_MS] [WORK_MS] [BEOBACHTUNGSDAUER_S]

set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODULE=wqt_demo.ko

PERIOD_MS="${1:-500}"
WORK_MS="${2:-100}"
OBSERVE_S="${3:-5}"

if [[ $EUID -ne 0 ]]; then
	echo "Bitte als root ausführen (insmod/rmmod erfordern Rechte)." >&2
	exit 1
fi

echo "=== Build ==="
make -C "${MODULE_DIR}"

if lsmod | grep -q '^wqt_demo'; then
	echo "Modul bereits geladen, entferne es zuerst..."
	rmmod wqt_demo
fi

echo "=== modinfo ==="
modinfo "${MODULE_DIR}/${MODULE}"

echo "=== insmod (period_ms=${PERIOD_MS} work_ms=${WORK_MS}) ==="
insmod "${MODULE_DIR}/${MODULE}" period_ms="${PERIOD_MS}" work_ms="${WORK_MS}"

cleanup() {
	echo "=== rmmod ==="
	rmmod wqt_demo || true
}
trap cleanup EXIT

echo "=== Beobachtung (${OBSERVE_S}s) ==="
dmesg -C >/dev/null 2>&1 || true
sleep "${OBSERVE_S}"
dmesg | grep 'wqt_demo:' | tail -n 20

echo
echo "Aktuelle Parameter:"
cat /sys/module/wqt_demo/parameters/period_ms
cat /sys/module/wqt_demo/parameters/work_ms
