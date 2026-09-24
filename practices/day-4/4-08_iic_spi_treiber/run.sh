#!/bin/bash
set -e

make

echo
printf 'Build erfolgreich.\n'
printf 'Device-Tree- und Bus-Aliase:\n'
modinfo training_i2c_spi.ko | grep -E 'alias:.*(demo,i2c-sensor|demo,spi-sensor|demo-i2c-sensor|demo-spi-sensor)'
printf 'Modul-Metadaten erfolgreich geprüft.\n'
