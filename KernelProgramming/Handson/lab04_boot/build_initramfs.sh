#!/bin/bash
# build_initramfs.sh — Create a minimal initramfs
#
# Usage: bash build_initramfs.sh
# Result: /tmp/custom_initramfs.cpio.gz

set -e

WORKDIR="/tmp/initramfs_build"
OUTPUT="/tmp/custom_initramfs.cpio.gz"

echo "=== Building Custom Initramfs ==="

# Clean
rm -rf "$WORKDIR"
mkdir -p "$WORKDIR"

# Create directory structure
echo "[1/5] Creating directory structure..."
mkdir -p "$WORKDIR"/{bin,sbin,etc,proc,sys,dev,newroot,tmp,lib,lib64}

# Install busybox
echo "[2/5] Installing BusyBox..."
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
cd "$WORKDIR/bin"
for cmd in sh ls cat echo mount umount mkdir mknod switch_root \
           sleep grep sed awk dmesg; do
    ln -sf busybox "$cmd" 2>/dev/null || true
done

# Create /init script
echo "[3/5] Creating /init script..."
cat > "$WORKDIR/init" << 'INITSCRIPT'
#!/bin/sh
echo "=== Initramfs Starting ==="

# Mount virtual filesystems
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev 2>/dev/null

echo "Kernel: $(cat /proc/version)"
echo ""

# Parse kernel command line for root=
ROOT_DEV=""
for param in $(cat /proc/cmdline); do
    case "$param" in
        root=*)
            ROOT_DEV="${param#root=}"
            ;;
    esac
done

if [ -n "$ROOT_DEV" ] && [ -e "$ROOT_DEV" ]; then
    echo "Mounting root device: $ROOT_DEV"
    mount "$ROOT_DEV" /newroot
    
    if [ -x /newroot/sbin/init ]; then
        echo "Switching to real root..."
        umount /proc /sys
        exec switch_root /newroot /sbin/init
    fi
fi

echo "No root device found or switch_root failed."
echo "Dropping to emergency shell..."
exec /bin/sh
INITSCRIPT
chmod +x "$WORKDIR/init"

# Create essential device nodes
echo "[4/5] Creating device nodes..."
sudo mknod "$WORKDIR/dev/console" c 5 1
sudo mknod "$WORKDIR/dev/null" c 1 3
sudo mknod "$WORKDIR/dev/tty" c 5 0

# Pack as cpio archive
echo "[5/5] Creating cpio archive..."
cd "$WORKDIR"
find . -print0 | cpio --null -ov --format=newc 2>/dev/null | gzip -9 > "$OUTPUT"

echo ""
echo "=== Initramfs Created ==="
echo "Output: $OUTPUT"
echo "Size:   $(du -sh $OUTPUT | cut -f1)"
echo ""
echo "Test with:"
echo "  qemu-system-x86_64 -kernel /path/to/bzImage -initrd $OUTPUT \\"
echo "    -append 'console=ttyS0 rdinit=/init' -nographic -m 256M"
