#!/bin/bash
# build_qemu_initramfs.sh — Create a test initramfs for QEMU booting
#
# Usage: bash build_qemu_initramfs.sh
# Result: /tmp/test_initramfs.cpio.gz

set -e

WORKDIR="/tmp/qemu_initramfs_build"
OUTPUT="/tmp/test_initramfs.cpio.gz"

echo "=== Building QEMU Test Initramfs ==="

rm -rf "$WORKDIR"
mkdir -p "$WORKDIR"/{bin,sbin,etc,proc,sys,dev,tmp}

# Install busybox
echo "[1/4] Installing BusyBox..."
if [ -f /bin/busybox ]; then
    cp /bin/busybox "$WORKDIR/bin/busybox"
elif command -v busybox &>/dev/null; then
    cp $(which busybox) "$WORKDIR/bin/busybox"
else
    sudo apt install -y busybox-static
    cp /bin/busybox "$WORKDIR/bin/busybox"
fi
chmod +x "$WORKDIR/bin/busybox"

# Create symlinks
echo "[2/4] Creating symlinks..."
cd "$WORKDIR/bin"
for cmd in sh ls cat echo mount umount mkdir mknod dmesg \
           sleep grep ps free top hostname date uname reboot poweroff; do
    ln -sf busybox "$cmd" 2>/dev/null || true
done

# Create init
echo "[3/4] Creating /init..."
cat > "$WORKDIR/init" << 'INITSCRIPT'
#!/bin/sh
echo ""
echo "========================================"
echo "  QEMU Test Kernel — Boot Successful!"
echo "========================================"
echo ""

mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev 2>/dev/null

echo "Kernel:  $(uname -r)"
echo "Arch:    $(uname -m)"
echo "Uptime:  $(cat /proc/uptime | cut -d' ' -f1)s"
echo "Memory:  $(grep MemTotal /proc/meminfo)"
echo "CPUs:    $(grep -c processor /proc/cpuinfo)"
echo ""
echo "Type 'poweroff' to exit QEMU"
echo ""

exec /bin/sh
INITSCRIPT
chmod +x "$WORKDIR/init"

# Device nodes
echo "[4/4] Creating device nodes..."
sudo mknod "$WORKDIR/dev/console" c 5 1
sudo mknod "$WORKDIR/dev/null" c 1 3
sudo mknod "$WORKDIR/dev/tty" c 5 0

# Pack
cd "$WORKDIR"
find . -print0 | cpio --null -ov --format=newc 2>/dev/null | gzip -9 > "$OUTPUT"

echo ""
echo "=== QEMU Initramfs Created ==="
echo "Output: $OUTPUT"
echo "Size:   $(du -sh $OUTPUT | cut -f1)"
echo ""
echo "Boot with:"
echo "  qemu-system-x86_64 \\"
echo "    -kernel /path/to/bzImage \\"
echo "    -initrd $OUTPUT \\"
echo "    -append 'console=ttyS0 rdinit=/init' \\"
echo "    -nographic -m 256M"
echo ""
echo "Exit QEMU: Ctrl+A then X"
