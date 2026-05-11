#!/bin/bash
# ftrace_demo.sh — Ftrace and perf demonstration script 🍓
#
# Usage: sudo ./ftrace_demo.sh
#
# Demonstrates:
#   1. Function tracing
#   2. Function graph tracing
#   3. Event tracing
#   4. Basic perf stat/record

set -e

TRACEDIR="/sys/kernel/tracing"

# Check if running as root
if [ "$(id -u)" -ne 0 ]; then
    echo "Error: Run as root (sudo ./ftrace_demo.sh)"
    exit 1
fi

# Mount tracefs if needed
mount -t tracefs nodev "$TRACEDIR" 2>/dev/null || true

echo "============================================"
echo "  Ftrace & Perf Demonstration"
echo "============================================"
echo ""

# --- 1. Function Tracing ---
echo "=== 1. Function Tracing (VFS functions) ==="
echo nop > "$TRACEDIR/current_tracer"
echo > "$TRACEDIR/trace"
echo 'vfs_*' > "$TRACEDIR/set_ftrace_filter"
echo function > "$TRACEDIR/current_tracer"
echo 1 > "$TRACEDIR/tracing_on"

# Generate VFS activity
cat /etc/hostname > /dev/null 2>&1
ls /tmp > /dev/null 2>&1

echo 0 > "$TRACEDIR/tracing_on"
echo "Trace output (first 20 lines):"
head -25 "$TRACEDIR/trace"
echo ""

# --- 2. Function Graph Tracing ---
echo "=== 2. Function Graph Tracing ==="
echo nop > "$TRACEDIR/current_tracer"
echo > "$TRACEDIR/trace"
echo > "$TRACEDIR/set_ftrace_filter"
echo function_graph > "$TRACEDIR/current_tracer"
# Only trace VFS read path
echo 'vfs_read' > "$TRACEDIR/set_graph_function" 2>/dev/null || true
echo 1 > "$TRACEDIR/tracing_on"

cat /etc/hostname > /dev/null 2>&1

echo 0 > "$TRACEDIR/tracing_on"
echo "Function graph (first 30 lines):"
head -35 "$TRACEDIR/trace"
echo ""

# --- 3. Event Tracing ---
echo "=== 3. Scheduler Event Tracing ==="
echo nop > "$TRACEDIR/current_tracer"
echo > "$TRACEDIR/trace"
echo > "$TRACEDIR/set_graph_function" 2>/dev/null || true
echo 1 > "$TRACEDIR/events/sched/sched_switch/enable"
echo 1 > "$TRACEDIR/tracing_on"

sleep 1

echo 0 > "$TRACEDIR/tracing_on"
echo 0 > "$TRACEDIR/events/sched/sched_switch/enable"
echo "Scheduler switches (first 20 lines of trace data):"
grep -v "^#" "$TRACEDIR/trace" | head -20
echo ""

# --- Cleanup ---
echo nop > "$TRACEDIR/current_tracer"
echo > "$TRACEDIR/set_ftrace_filter"
echo > "$TRACEDIR/set_graph_function" 2>/dev/null || true
echo > "$TRACEDIR/trace"

# --- 4. Perf stat ---
echo "=== 4. Perf stat (ls /tmp) ==="
if command -v perf &>/dev/null; then
    perf stat -e cycles,instructions,cache-misses,branch-misses ls /tmp > /dev/null 2>&1 || \
        perf stat ls /tmp > /dev/null
else
    echo "(perf not installed — skip)"
fi
echo ""

echo "=== Done ==="
echo ""
echo "Next steps:"
echo "  trace-cmd record -e sched_switch sleep 5"
echo "  trace-cmd report | head -30"
echo "  sudo perf record -g -a sleep 5"
echo "  sudo perf report"
