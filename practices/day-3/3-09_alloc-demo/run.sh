#!/usr/bin/env bash
#
# run.sh - Baut alloc_demo.ko und führt die Testmatrix aus Folie 14 aus:
# Normalfall, ungültige Parameter (-EINVAL) und simulierter Teilfehler
# (fail_after, -ENOMEM).
#
# Aufruf: sudo ./run.sh

set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODULE=alloc_demo.ko

if [[ $EUID -ne 0 ]]; then
	echo "Bitte als root ausführen (insmod/rmmod erfordern Rechte)." >&2
	exit 1
fi

echo "=== Build ==="
make -C "${MODULE_DIR}"

unload_if_loaded() {
	if lsmod | grep -q '^alloc_demo'; then
		rmmod alloc_demo
	fi
}
unload_if_loaded

run_case() {
	local desc="$1"
	shift
	echo
	echo "=== ${desc} (insmod $* ) ==="
	if insmod "${MODULE_DIR}/${MODULE}" "$@" 2>/tmp/alloc_demo_err; then
		dmesg | tail -n 5
		rmmod alloc_demo
	else
		echo "insmod fehlgeschlagen (erwartet): $(cat /tmp/alloc_demo_err)"
		dmesg | tail -n 5
	fi
}

run_case "Normalfall"              count=4 payload_size=128
run_case "count=0 -> -EINVAL"      count=0
run_case "payload_size=0 -> -EINVAL" payload_size=0
run_case "fail_after=3 -> -ENOMEM" count=8 fail_after=3

echo
echo "=== 50x Laden/Entladen ==="
for i in $(seq 1 50); do
	insmod "${MODULE_DIR}/${MODULE}" count=4 payload_size=128
	rmmod alloc_demo
done
echo "50 Zyklen abgeschlossen, letzte Log-Zeilen:"
dmesg | tail -n 10

rm -f /tmp/alloc_demo_err
