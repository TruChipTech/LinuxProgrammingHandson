#!/bin/bash
# explore_dt.sh — Device Tree Exploration Script
# Run on a Linux target (RPi or VM with DT support)

set -e

echo "=== Device Tree Exploration ==="
echo

DT_BASE="/proc/device-tree"
if [ ! -d "$DT_BASE" ]; then
    DT_BASE="/sys/firmware/devicetree/base"
fi

if [ ! -d "$DT_BASE" ]; then
    echo "ERROR: Device tree not found at /proc/device-tree or /sys/firmware/devicetree/base"
    echo "This system may use ACPI instead of Device Tree."
    exit 1
fi

echo "Device Tree base: $DT_BASE"
echo

# 1. Model
echo "--- Board Model ---"
if [ -f "$DT_BASE/model" ]; then
    cat "$DT_BASE/model" ; echo
else
    echo "(no model property)"
fi

# 2. Compatible
echo
echo "--- Root Compatible ---"
cat "$DT_BASE/compatible" | tr '\0' '\n'

# 3. Top-level nodes
echo
echo "--- Top-Level Nodes ---"
ls "$DT_BASE/" | head -30

# 4. SoC devices (if exists)
echo
echo "--- SoC Devices ---"
if [ -d "$DT_BASE/soc" ]; then
    ls "$DT_BASE/soc/" | head -30
else
    echo "(no /soc node)"
fi

# 5. Memory info
echo
echo "--- Memory ---"
if [ -d "$DT_BASE/memory" ] || [ -d "$DT_BASE/memory@0" ]; then
    MEM_NODE=$(ls -d "$DT_BASE"/memory* 2>/dev/null | head -1)
    echo "Node: $(basename $MEM_NODE)"
    if [ -f "$MEM_NODE/reg" ]; then
        echo "reg: $(hexdump -C "$MEM_NODE/reg" | head -3)"
    fi
fi

# 6. Decompile to DTS (if dtc available)
echo
echo "--- Decompile Attempt ---"
if command -v dtc &>/dev/null; then
    echo "dtc found — decompiling first 50 lines:"
    dtc -I fs "$DT_BASE/" 2>/dev/null | head -50
else
    echo "dtc not installed — install with: sudo apt install device-tree-compiler"
fi

echo
echo "=== Done ==="
