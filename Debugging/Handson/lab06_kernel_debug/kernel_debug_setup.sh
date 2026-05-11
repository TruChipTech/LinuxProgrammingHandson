#!/bin/bash
# kernel_debug_setup.sh — Kernel debugging setup and exploration
#
# Covers: KGDB setup, KDB, crash dump configuration, LTTng
# Usage: sudo ./kernel_debug_setup.sh

set -e

if [ "$(id -u)" -ne 0 ]; then
    echo "Run as root: sudo $0"
    exit 1
fi

echo "============================================"
echo "  Kernel Debugging Environment Setup"
echo "============================================"
echo ""

# --- 1. Check KGDB / KDB support ---
echo "=== 1. KGDB / KDB Kernel Config ==="
CONFIG="/boot/config-$(uname -r)"
if [ -f "$CONFIG" ]; then
    echo "Checking $CONFIG:"
    for opt in KGDB KGDB_SERIAL_CONSOLE KGDB_KDB MAGIC_SYSRQ DEBUG_INFO \
               FRAME_POINTER KALLSYMS CRASH_DUMP PROC_KCORE KEXEC; do
        val=$(grep "^CONFIG_$opt=" "$CONFIG" 2>/dev/null || echo "not set")
        printf "  %-30s %s\n" "CONFIG_$opt" "$val"
    done
else
    echo "  (config file not found)"
    if [ -f /proc/config.gz ]; then
        echo "  Trying /proc/config.gz..."
        zcat /proc/config.gz | grep -E "KGDB|KDB|MAGIC_SYSRQ|DEBUG_INFO" | head -10
    fi
fi
echo ""

# --- 2. Magic SysRq ---
echo "=== 2. Magic SysRq Status ==="
SYSRQ=$(cat /proc/sys/kernel/sysrq)
echo "  Current sysrq value: $SYSRQ"
if [ "$SYSRQ" -eq 0 ]; then
    echo "  SysRq is DISABLED. Enable with:"
    echo "    echo 1 > /proc/sys/kernel/sysrq"
elif [ "$SYSRQ" -eq 1 ]; then
    echo "  SysRq is FULLY ENABLED"
else
    echo "  SysRq is partially enabled (bitmask: $SYSRQ)"
fi
echo ""

# --- 3. Crash dump (kdump) status ---
echo "=== 3. Crash Dump (kdump) Status ==="
if command -v kdump-config &>/dev/null; then
    kdump-config show 2>/dev/null || echo "  kdump not configured"
elif systemctl is-active kdump 2>/dev/null; then
    echo "  kdump service is active"
else
    echo "  kdump not installed or not active"
    echo "  Install: sudo apt install kdump-tools kexec-tools"
fi

CRASHKERNEL=$(cat /proc/cmdline | grep -o 'crashkernel=[^ ]*' || true)
if [ -n "$CRASHKERNEL" ]; then
    echo "  Crash kernel reservation: $CRASHKERNEL"
    echo "  Reserved size: $(cat /sys/kernel/kexec_crash_size 2>/dev/null || echo 'unknown') bytes"
else
    echo "  No crashkernel= parameter in boot cmdline"
fi
echo ""

# --- 4. Debug symbols ---
echo "=== 4. Debug Symbols ==="
if [ -f "/usr/lib/debug/boot/vmlinux-$(uname -r)" ]; then
    echo "  Debug vmlinux: FOUND"
else
    echo "  Debug vmlinux: NOT FOUND"
    echo "  Install: sudo apt install linux-image-$(uname -r)-dbg"
fi
echo ""

# --- 5. Ftrace availability ---
echo "=== 5. Ftrace Status ==="
if [ -d /sys/kernel/tracing ]; then
    echo "  Tracefs: mounted at /sys/kernel/tracing"
    echo "  Available tracers: $(cat /sys/kernel/tracing/available_tracers)"
    echo "  Current tracer: $(cat /sys/kernel/tracing/current_tracer)"
    echo "  Traceable functions: $(wc -l < /sys/kernel/tracing/available_filter_functions)"
    echo "  Event categories: $(ls /sys/kernel/tracing/events/ | wc -l)"
elif [ -d /sys/kernel/debug/tracing ]; then
    echo "  Tracefs: mounted at /sys/kernel/debug/tracing"
else
    echo "  Tracefs: NOT MOUNTED"
    echo "  Mount: sudo mount -t tracefs nodev /sys/kernel/tracing"
fi
echo ""

# --- 6. Perf availability ---
echo "=== 6. Perf Tool ==="
if command -v perf &>/dev/null; then
    perf --version
    echo "  Hardware events: $(perf list hw 2>/dev/null | grep -c 'Hardware')"
else
    echo "  perf not installed"
    echo "  Install: sudo apt install linux-tools-$(uname -r)"
fi
echo ""

# --- 7. LTTng availability ---
echo "=== 7. LTTng ==="
if command -v lttng &>/dev/null; then
    echo "  LTTng version: $(lttng --version)"
    lsmod | grep lttng && echo "  LTTng modules loaded" || echo "  LTTng modules not loaded"
else
    echo "  LTTng not installed"
    echo "  Install: sudo apt install lttng-tools lttng-modules-dkms babeltrace2"
fi
echo ""

# --- 8. eBPF availability ---
echo "=== 8. eBPF ==="
if command -v bpftrace &>/dev/null; then
    echo "  bpftrace: $(bpftrace --version 2>&1)"
else
    echo "  bpftrace not installed"
fi
if command -v bpftool &>/dev/null; then
    echo "  bpftool: available"
else
    echo "  bpftool: not installed"
fi
echo ""

echo "=== Setup Complete ==="
echo ""
echo "To enable all debugging features, add to GRUB_CMDLINE_LINUX:"
echo '  "nokaslr kgdboc=ttyS0,115200 crashkernel=256M"'
echo "Then run: sudo update-grub && sudo reboot"
