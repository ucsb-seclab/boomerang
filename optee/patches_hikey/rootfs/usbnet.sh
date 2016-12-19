#!/bin/sh
#
# Script to bring eth0 up and start DHCP client
# Run it after plugging a USB ethernet adapter, for instance

ip link set eth0 up
udhcpc -i eth0 -s /etc/udhcp/simple.script
