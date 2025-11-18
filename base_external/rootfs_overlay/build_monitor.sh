#!/bin/sh

echo "Building system monitor daemon..."

aarch64-linux-gnu-gcc -static -o base_external/rootfs_overlay/usr/bin/system_monitor \
    base_external/rootfs_overlay/usr/bin/system_monitor.c

if [ $? -eq 0 ]; then
    echo "System monitor daemon built successfully!"
    chmod +x base_external/rootfs_overlay/usr/bin/system_monitor
    chmod +x base_external/rootfs_overlay/etc/init.d/S60systemmonitor
else
    echo "Failed to build system monitor daemon!"
    exit 1
fi