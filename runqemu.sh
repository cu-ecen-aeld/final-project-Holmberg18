#!/bin/bash
#Script to run QEMU for buildroot as the default configuration qemu_aesd_rpi4_defconfig
#Author: Jon Holmberg

set -e
cd `dirname $0`

echo "=== Starting QEMU Test ==="
echo "Image: buildroot/output/images/rootfs.ext4"
echo "Kernel: buildroot/output/images/Image"

# Check if required files exist
if [ ! -f "buildroot/output/images/Image" ]; then
    echo "ERROR: Kernel Image not found!"
    exit 1
fi

if [ ! -f "buildroot/output/images/rootfs.ext4" ]; then
    echo "ERROR: Root filesystem not found!"
    exit 1
fi

echo "Starting QEMU..."
echo "To exit: Press Ctrl+A, then X"
echo ""

qemu-system-aarch64 \
  -M virt \
  -cpu cortex-a72 \
  -smp 4 \
  -m 2G \
  -nographic \
  -kernel buildroot/output/images/Image \
  -append "root=/dev/vda console=ttyAMA0" \
  -drive file=buildroot/output/images/rootfs.ext4,if=virtio,format=raw \
  -netdev user,id=eth0,hostfwd=tcp::8080-:80 \
  -device virtio-net-device,netdev=eth0

echo "=== QEMU Session Ended ==="