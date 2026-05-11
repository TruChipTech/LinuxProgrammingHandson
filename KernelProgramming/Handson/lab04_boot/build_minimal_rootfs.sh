#!/bin/bash
# build_minimal_rootfs.sh — Create a minimal root filesystem
#
# Usage: bash build_minimal_rootfs.sh
# Result: /tmp/minimal_rootfs/ with a bootable minimal rootfs

set -e

ROOTFS="/tmp/minimal_rootfs"
echo "=== Building Minimal Root Filesystem ==="

# Clean up
sudo rm -rf "$ROOTFS"
mkdir -p "$ROOTFS"

# Create FHS directory structure
echo "[1/6] Creating directory structure..."
mkdir -p "$ROOTFS"/{bin,sbin,etc,proc,sys,dev,tmp,var/log,usr/bin,usr/sbin,lib,lib64}

# Install busybox (static binary — no shared libs needed)
echo "[2/6] Installing BusyBox..."
if command -v busybox &>/dev/null; then
    cp $(which busybox) "$ROOTFS/bin/busybox"
else
    echo "  BusyBox not found, installing..."
    sudo apt install -y busybox-static
    cp /bin/busybox "$ROOTFS/bin/busybox"
fi

# Create symlinks for common commands
echo "[3/6] Creating command symlinks..."
cd "$ROOTFS/bin"
for cmd in sh ash ls cat echo mkdir mount umount ps kill sleep \
           cp mv rm ln grep sed awk head tail wc date hostname \
           dmesg df free top vi; do
    ln -sf busybox "$cmd" 2>/dev/null || true
done
cd "$ROOTFS/sbin"
for cmd in init halt reboot poweroff mdev; do
    ln -sf ../bin/busybox "$cmd" 2>/dev/null || true
done

# Create init script
echo "[4/6] Creating init script..."
cat > "$ROOTFS/etc/inittab" << 'INITTAB'
::sysinit:/etc/init.d/rcS
::respawn:-/bin/sh
::ctrlaltdel:/sbin/reboot
::shutdown:/bin/umount -a -r
INITTAB

mkdir -p "$ROOTFS/etc/init.d"
cat > "$ROOTFS/etc/init.d/rcS" << 'INIT'
#!/bin/sh
echo "=== Minimal Linux System Starting ==="

# Mount virtual filesystems
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev 2>/dev/null || true

# Set hostname
hostname minimal-linux
echo "minimal-linux" > /etc/hostname

# Show system info
echo "Kernel: $(uname -r)"
echo "Hostname: $(hostname)"
echo "Date: $(date)"
echo ""
echo "=== System Ready ==="
INIT
chmod +x "$ROOTFS/etc/init.d/rcS"

# Create essential device nodes
echo "[5/6] Creating device nodes..."
sudo mknod "$ROOTFS/dev/console" c 5 1
sudo mknod "$ROOTFS/dev/null" c 1 3
sudo mknod "$ROOTFS/dev/zero" c 1 5
sudo mknod "$ROOTFS/dev/tty" c 5 0

# Create basic config files
echo "[6/6] Creating config files..."
echo "root:x:0:0:root:/:/bin/sh" > "$ROOTFS/etc/passwd"
echo "root:x:0:" > "$ROOTFS/etc/group"
echo "minimal-linux" > "$ROOTFS/etc/hostname"

echo ""
echo "=== Minimal Rootfs Complete ==="
echo "Location: $ROOTFS"
echo "Size: $(du -sh $ROOTFS | cut -f1)"
echo "Files: $(find $ROOTFS -type f | wc -l)"
echo ""
echo "Contents:"
find "$ROOTFS" -maxdepth 2 | head -30
