#!/bin/bash
# explore_alsa.sh — ALSA Audio Subsystem Exploration 🍓
#
# Run on Raspberry Pi or any Linux system with audio hardware.
# Usage: chmod +x explore_alsa.sh && sudo ./explore_alsa.sh

set -e

echo "============================================"
echo "  ALSA Audio Subsystem Explorer"
echo "============================================"
echo ""

echo "--- 1. Sound Cards ---"
cat /proc/asound/cards 2>/dev/null || echo "(no cards found)"
echo ""

echo "--- 2. PCM Devices ---"
cat /proc/asound/pcm 2>/dev/null || echo "(no PCM devices)"
echo ""

echo "--- 3. ALSA Playback Devices (aplay -l) ---"
aplay -l 2>/dev/null || echo "(aplay not found — install alsa-utils)"
echo ""

echo "--- 4. ALSA Capture Devices (arecord -l) ---"
arecord -l 2>/dev/null || echo "(arecord not found)"
echo ""

echo "--- 5. Mixer Controls ---"
amixer 2>/dev/null | head -60 || echo "(amixer not found)"
echo ""

echo "--- 6. Kernel Sound Modules ---"
lsmod | grep -i snd || echo "(no snd modules loaded)"
echo ""

echo "--- 7. Sound Device Tree Nodes (RPi) ---"
if [ -d /proc/device-tree/sound ]; then
    echo "Sound node found:"
    ls -la /proc/device-tree/sound/
    echo "Compatible:"
    cat /proc/device-tree/sound/compatible 2>/dev/null && echo ""
else
    echo "(no /proc/device-tree/sound node)"
fi
echo ""

echo "--- 8. ALSA sysfs Info ---"
if [ -d /sys/class/sound ]; then
    for card in /sys/class/sound/card*; do
        [ -d "$card" ] || continue
        echo "Card: $(basename $card)"
        cat "$card/id" 2>/dev/null && echo ""
    done
else
    echo "(no /sys/class/sound)"
fi
echo ""

echo "--- 9. ASoC (Advanced Sound Architecture) ---"
if [ -d /sys/kernel/debug/asoc ]; then
    echo "ASoC platforms:"
    ls /sys/kernel/debug/asoc/ 2>/dev/null || echo "(access denied — run as root)"
else
    echo "(ASoC debugfs not available)"
fi
echo ""

echo "--- 10. Quick Audio Test ---"
echo "To test audio output:"
echo "  speaker-test -t sine -f 440 -c 2 -l 1"
echo "To record and play back:"
echo "  arecord -d 3 -f cd test.wav && aplay test.wav"
echo ""
echo "Done!"
