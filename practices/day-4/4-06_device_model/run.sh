#!/bin/bash
set -e

make
sudo insmod training_device.ko
sudo insmod training_driver.ko
sudo dmesg | tail -n 20
readlink /sys/bus/platform/devices/training-led/driver
sudo rmmod training_driver
sudo rmmod training_device
