#!/bin/bash
# ebpf_oneliners.sh — Collection of useful eBPF one-liners 🍓
#
# Usage: sudo ./ebpf_oneliners.sh <number>
#
# Each one-liner runs for ~10 seconds then stops.

if [ "$(id -u)" -ne 0 ]; then
    echo "Run as root: sudo $0 <number>"
    exit 1
fi

DEMO=${1:-0}

echo "eBPF One-Liner Collection"
echo "========================="
echo ""

case $DEMO in
1)
    echo "[1] Top syscall-making processes (10s):"
    timeout 10 bpftrace -e '
    tracepoint:raw_syscalls:sys_enter { @[comm] = count(); }
    interval:s:10 { exit(); }' 2>/dev/null
    ;;
2)
    echo "[2] Trace file opens with filenames (10s):"
    timeout 10 bpftrace -e '
    tracepoint:syscalls:sys_enter_openat {
        printf("%-16s %s\n", comm, str(args.filename));
    }' 2>/dev/null
    ;;
3)
    echo "[3] Read size histogram (10s):"
    timeout 10 bpftrace -e '
    tracepoint:syscalls:sys_exit_read /args.ret > 0/ {
        @bytes = hist(args.ret);
    }' 2>/dev/null
    ;;
4)
    echo "[4] Process creation tracing (10s):"
    timeout 10 bpftrace -e '
    tracepoint:sched:sched_process_exec {
        printf("exec: %s pid=%d\n", comm, pid);
    }' 2>/dev/null
    ;;
5)
    echo "[5] TCP connect tracing (10s):"
    timeout 10 bpftrace -e '
    kprobe:tcp_v4_connect { @connects[comm] = count(); }
    interval:s:10 { exit(); }' 2>/dev/null
    ;;
6)
    echo "[6] Block I/O latency histogram (10s):"
    timeout 10 bpftrace -e '
    tracepoint:block:block_rq_issue { @start[args.dev, args.sector] = nsecs; }
    tracepoint:block:block_rq_complete /@start[args.dev, args.sector]/ {
        @usecs = hist((nsecs - @start[args.dev, args.sector]) / 1000);
        delete(@start[args.dev, args.sector]);
    }' 2>/dev/null
    ;;
7)
    echo "[7] Scheduler latency (run queue time) (10s):"
    timeout 10 bpftrace -e '
    tracepoint:sched:sched_wakeup { @qtime[args.pid] = nsecs; }
    tracepoint:sched:sched_switch /@qtime[args.next_pid]/ {
        @usecs = hist((nsecs - @qtime[args.next_pid]) / 1000);
        delete(@qtime[args.next_pid]);
    }' 2>/dev/null
    ;;
*)
    echo "Usage: $0 <1-7>"
    echo ""
    echo "  1  Top syscall makers"
    echo "  2  File open tracing"
    echo "  3  Read size histogram"
    echo "  4  Process creation"
    echo "  5  TCP connections"
    echo "  6  Block I/O latency"
    echo "  7  Scheduler latency"
    ;;
esac

echo ""
echo "Done."
