#!/bin/bash
# network_debug.sh — Network debugging toolkit 🍓
#
# Demonstrates tcpdump, tshark, and network analysis
# Usage: sudo ./network_debug.sh [demo_number]

set -e

if [ "$(id -u)" -ne 0 ]; then
    echo "Run as root: sudo $0 [demo_number]"
    exit 1
fi

DEMO=${1:-0}
IFACE=$(ip route | grep default | awk '{print $5}' | head -1)
IFACE=${IFACE:-eth0}

echo "============================================"
echo "  Network Debugging Toolkit"
echo "  Interface: $IFACE"
echo "============================================"
echo ""

case $DEMO in
1)
    echo "=== [1] Basic tcpdump Capture (20 packets) ==="
    tcpdump -i "$IFACE" -n -c 20
    ;;
2)
    echo "=== [2] HTTP Traffic Capture ==="
    echo "Capturing port 80 traffic for 15s..."
    timeout 15 tcpdump -i "$IFACE" -n port 80 -w /tmp/http_capture.pcap &
    PID=$!
    sleep 2
    curl -s http://example.com > /dev/null 2>&1 || true
    wait $PID 2>/dev/null || true
    echo "Saved to /tmp/http_capture.pcap"
    echo "Packets captured:"
    tcpdump -r /tmp/http_capture.pcap -n | wc -l
    ;;
3)
    echo "=== [3] DNS Traffic Analysis ==="
    echo "Capturing DNS for 10s..."
    timeout 10 tcpdump -i "$IFACE" -n port 53 -w /tmp/dns_capture.pcap &
    PID=$!
    sleep 1
    dig example.com > /dev/null 2>&1 || true
    dig google.com > /dev/null 2>&1 || true
    wait $PID 2>/dev/null || true
    echo ""
    echo "DNS packets:"
    tcpdump -r /tmp/dns_capture.pcap -n 2>/dev/null
    ;;
4)
    echo "=== [4] TCP Flags Analysis ==="
    echo "Capturing SYN/FIN/RST packets for 15s..."
    timeout 15 tcpdump -i "$IFACE" -n \
        'tcp[tcpflags] & (tcp-syn|tcp-fin|tcp-rst) != 0' -c 20
    ;;
5)
    echo "=== [5] tshark Protocol Statistics ==="
    if ! command -v tshark &>/dev/null; then
        echo "tshark not installed. Install: sudo apt install tshark"
        exit 1
    fi
    echo "Capturing for 10s..."
    timeout 10 tshark -i "$IFACE" -w /tmp/stats_capture.pcap -q &
    PID=$!
    sleep 2
    curl -s http://example.com > /dev/null 2>&1 || true
    ping -c 3 8.8.8.8 > /dev/null 2>&1 || true
    wait $PID 2>/dev/null || true
    echo ""
    echo "Protocol Hierarchy:"
    tshark -r /tmp/stats_capture.pcap -q -z io,phs 2>/dev/null
    echo ""
    echo "Conversations:"
    tshark -r /tmp/stats_capture.pcap -q -z conv,tcp 2>/dev/null
    ;;
6)
    echo "=== [6] Connection State Analysis ==="
    echo "Current TCP connections:"
    ss -tnp | head -20
    echo ""
    echo "Connection state summary:"
    ss -s
    echo ""
    echo "Listening ports:"
    ss -tlnp
    ;;
7)
    echo "=== [7] Network Latency Test ==="
    echo "Ping latency to common targets:"
    for target in 8.8.8.8 1.1.1.1 127.0.0.1; do
        echo -n "  $target: "
        ping -c 3 -q "$target" 2>/dev/null | tail -1 | awk -F'/' '{print $5 " ms avg"}' || echo "unreachable"
    done
    echo ""
    echo "MTU discovery:"
    ip link show "$IFACE" | grep mtu
    ;;
*)
    echo "Usage: $0 <1-7>"
    echo ""
    echo "  1  Basic tcpdump capture"
    echo "  2  HTTP traffic capture & analysis"
    echo "  3  DNS traffic analysis"
    echo "  4  TCP flags (SYN/FIN/RST)"
    echo "  5  tshark protocol statistics"
    echo "  6  Connection state analysis"
    echo "  7  Network latency test"
    echo ""
    echo "Files saved to /tmp/*_capture.pcap"
    echo "Open in Wireshark: wireshark /tmp/http_capture.pcap"
    ;;
esac

echo ""
echo "Done."
