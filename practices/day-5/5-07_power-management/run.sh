#!/bin/bash
#
# run.sh - Baut pm_demo_device.ko und pm_demo_driver.ko, lädt beide,
# prüft die sysfs-Bindung und demonstriert Autosuspend, balancierte
# Get/Put-Referenzen sowie ein simuliertes Wakeup-Event.
#
# Aufruf: sudo ./run.sh

set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEV="/sys/bus/platform/devices/pm-demo"

if [[ $EUID -ne 0 ]]; then
	echo "Bitte als root ausführen (insmod/rmmod erfordern Rechte)." >&2
	exit 1
fi

echo "=== Build ==="
make -C "${MODULE_DIR}"

cleanup() {
	if lsmod | grep -q '^pm_demo_driver'; then
		rmmod pm_demo_driver || true
	fi
	if lsmod | grep -q '^pm_demo_device'; then
		rmmod pm_demo_device || true
	fi
}
trap cleanup EXIT

if lsmod | grep -q '^pm_demo_driver'; then
	echo "Treiber bereits geladen, entferne ihn zuerst..."
	rmmod pm_demo_driver
fi
if lsmod | grep -q '^pm_demo_device'; then
	echo "Gerät bereits geladen, entferne es zuerst..."
	rmmod pm_demo_device
fi

echo "=== insmod ==="
insmod "${MODULE_DIR}/pm_demo_device.ko"
insmod "${MODULE_DIR}/pm_demo_driver.ko"
sleep 1

echo "=== Bindung prüfen ==="
readlink "${DEV}/driver"
cat "${DEV}/power/runtime_status"
cat "${DEV}/power/control"
cat "${DEV}/power/autosuspend_delay_ms"

echo
echo "=== Zugriff: get -> Zugriff -> put_autosuspend ==="
echo 1 > "${DEV}/access"
cat "${DEV}/access_count"
cat "${DEV}/power/runtime_status"

echo
echo "=== Autosuspend abwarten (>2s Delay) ==="
sleep 3
cat "${DEV}/power/runtime_status"
dmesg | tail -n 10

echo
echo "=== Erneuter Zugriff weckt das Gerät wieder auf ==="
echo 1 > "${DEV}/access"
cat "${DEV}/power/runtime_status"
cat "${DEV}/access_count"

echo
echo "=== Wakeup-Event simulieren ==="
cat "${DEV}/power/wakeup"
echo 1 > "${DEV}/simulate_wakeup"
cat "${DEV}/power/wakeup_count" 2>/dev/null || true
dmesg | tail -n 5

echo
echo "=== Rückbau ==="
trap - EXIT
rmmod pm_demo_driver
rmmod pm_demo_device
echo "Fertig."
