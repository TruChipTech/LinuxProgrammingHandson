#!/bin/bash
# explore_v4l2.sh — V4L2 Camera Subsystem Exploration 🍓
#
# Run on Raspberry Pi or any Linux with a camera.
# Usage: chmod +x explore_v4l2.sh && ./explore_v4l2.sh

set -e

echo "============================================"
echo "  V4L2 Camera Subsystem Explorer"
echo "============================================"
echo ""

echo "--- 1. Video Devices ---"
ls -la /dev/video* 2>/dev/null || echo "(no video devices found)"
echo ""

echo "--- 2. V4L2 Device Info ---"
for dev in /dev/video*; do
    [ -c "$dev" ] || continue
    echo "=== $dev ==="
    v4l2-ctl -d "$dev" --info 2>/dev/null || echo "  (cannot query)"
    echo ""
done

echo "--- 3. Supported Formats (video0) ---"
v4l2-ctl -d /dev/video0 --list-formats-ext 2>/dev/null || echo "(not available)"
echo ""

echo "--- 4. Current Settings (video0) ---"
v4l2-ctl -d /dev/video0 --all 2>/dev/null | head -40 || echo "(not available)"
echo ""

echo "--- 5. Camera Kernel Modules ---"
lsmod | grep -iE "bcm2835|uvc|videobuf|v4l2|camera" || echo "(no camera modules)"
echo ""

echo "--- 6. Media Devices ---"
ls -la /dev/media* 2>/dev/null || echo "(no media devices)"
if command -v media-ctl &>/dev/null; then
    echo ""
    media-ctl -p 2>/dev/null | head -30 || true
fi
echo ""

echo "--- 7. Camera Device Tree (RPi) ---"
for path in /proc/device-tree/soc/csi* /proc/device-tree/soc/i2c*/ov5647* \
            /proc/device-tree/soc/i2c*/imx*; do
    [ -d "$path" ] || continue
    echo "DT node: $(echo $path | sed 's|/proc/device-tree||')"
    cat "$path/compatible" 2>/dev/null && echo ""
done
echo ""

echo "--- 8. Quick Capture Test ---"
echo "Commands to try:"
echo "  v4l2-ctl --set-fmt-video=width=640,height=480,pixelformat=YUYV"
echo "  v4l2-ctl --stream-mmap --stream-count=1 --stream-to=frame.raw"
echo "  libcamera-still -o test.jpg        # RPi Camera Module"
echo "  ffplay /dev/video0                  # Live preview"
echo ""
echo "Done!"
