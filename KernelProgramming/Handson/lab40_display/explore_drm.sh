#!/bin/bash
# explore_drm.sh — DRM/KMS Display Subsystem Exploration 🍓
#
# Run on Raspberry Pi or any Linux system with a display.
# Usage: chmod +x explore_drm.sh && sudo ./explore_drm.sh

set -e

echo "============================================"
echo "  DRM/KMS Display Subsystem Explorer"
echo "============================================"
echo ""

echo "--- 1. DRM Devices ---"
ls -la /dev/dri/ 2>/dev/null || echo "(no DRM devices found)"
echo ""

echo "--- 2. DRM Card Info (sysfs) ---"
for card in /sys/class/drm/card*; do
    [ -d "$card" ] || continue
    name=$(basename "$card")
    echo "=== $name ==="
    [ -f "$card/device/driver" ] && echo "  Driver: $(basename $(readlink "$card/device/driver"))"

    # Connectors
    for conn in "$card"-*; do
        [ -d "$conn" ] || continue
        cname=$(basename "$conn")
        status=$(cat "$conn/status" 2>/dev/null || echo "unknown")
        echo "  Connector: $cname — $status"
        if [ "$status" = "connected" ]; then
            [ -f "$conn/modes" ] && echo "    Modes:" && head -5 "$conn/modes" | sed 's/^/      /'
        fi
    done
    echo ""
done

echo "--- 3. Framebuffer Devices ---"
ls -la /dev/fb* 2>/dev/null || echo "(no framebuffer devices)"
if [ -f /sys/class/graphics/fb0/virtual_size ]; then
    echo "  fb0 virtual_size: $(cat /sys/class/graphics/fb0/virtual_size)"
    echo "  fb0 bits_per_pixel: $(cat /sys/class/graphics/fb0/bits_per_pixel)"
fi
echo ""

echo "--- 4. GPU/Display Kernel Modules ---"
lsmod | grep -iE "drm|gpu|vc4|v3d|hdmi|panel" || echo "(no display modules found)"
echo ""

echo "--- 5. Display Device Tree (RPi) ---"
for node in hdmi panel dsi gpu v3d; do
    path="/proc/device-tree/$node"
    [ -d "$path" ] && echo "DT node: /$node" && \
        cat "$path/compatible" 2>/dev/null && echo ""
done
for path in /proc/device-tree/soc/gpu* /proc/device-tree/soc/hdmi*; do
    [ -d "$path" ] || continue
    echo "DT node: $(echo $path | sed 's|/proc/device-tree||')"
    cat "$path/compatible" 2>/dev/null && echo ""
done
echo ""

echo "--- 6. DRM Debug Info ---"
if [ -d /sys/kernel/debug/dri ]; then
    for card in /sys/kernel/debug/dri/*/name; do
        [ -f "$card" ] && echo "  $(cat $card)"
    done
else
    echo "(debugfs not available — run as root)"
fi
echo ""

echo "--- 7. Resolution & Display Tools ---"
echo "Useful commands:"
echo "  modetest -M vc4     # List modes (RPi)"
echo "  drm_info             # Detailed DRM info (install drm_info)"
echo "  xrandr               # X11 display info"
echo "  wlr-randr            # Wayland display info"
echo "  tvservice -s         # RPi HDMI status (legacy)"
echo ""
echo "Done!"
