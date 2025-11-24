#!/bin/bash
# Script to build for Raspberry Pi 4 hardware
# Author: Jon Holmberg

set -e 
cd "$(dirname "$0")"
chmod +x base_external/rootfs_overlay/etc/init.d/S50lighttpd
chmod +x base_external/rootfs_overlay/etc/init.d/S60systemmonitor
chmod +x base_external/rootfs_overlay/usr/bin/system_monitor
chmod 644 base_external/rootfs_overlay/var/www/index.html
chmod 644 base_external/rootfs_overlay/var/www/styles.css
chmod 644 base_external/rootfs_overlay/var/www/script.js
chmod 644 base_external/rootfs_overlay/etc/network/interfaces
chmod 600 base_external/rootfs_overlay/etc/wpa_supplicant.conf
echo "=== Building for Raspberry Pi 4 Hardware ==="

# Clean previous config and build
echo "Step 1: Cleaning previous build..."
make -C buildroot distclean

# Use RPi4 defconfig
if [ -e "base_external/configs/aesd_rpi4_defconfig" ]; then
    echo "Step 2: Configuring with aesd_rpi4_defconfig"
    make -C buildroot defconfig BR2_EXTERNAL="../base_external" BR2_DEFCONFIG="../base_external/configs/aesd_rpi4_defconfig"
else
    echo "ERROR: aesd_rpi4_defconfig not found!"
    exit 1
fi

# Build for RPi4
echo "Step 4: Building RPi4 image (this will take a while)..."
make -C buildroot BR2_EXTERNAL="../base_external"

echo "=== RPi4 Build Complete ==="
echo "Output files in buildroot/output/images/"
echo ""
echo "Ready to deploy to SD card:"