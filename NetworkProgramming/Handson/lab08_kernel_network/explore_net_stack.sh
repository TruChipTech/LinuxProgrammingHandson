#!/bin/bash
# explore_net_stack.sh — Explore the Linux kernel network stack
#
# Run: sudo bash explore_net_stack.sh

set -e

echo "============================================="
echo "  Linux Kernel Network Stack Explorer"
echo "============================================="
echo ""

# Check root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root: sudo bash $0"
    exit 1
fi

echo "=== 1. Network Interfaces ==="
ip -s link show
echo ""

echo "=== 2. IP Addresses ==="
ip -4 addr show
echo ""

echo "=== 3. Routing Table ==="
ip route show
echo ""

echo "=== 4. ARP Cache ==="
ip neigh show
echo ""

echo "=== 5. TCP Connections ==="
ss -tnp
echo ""

echo "=== 6. UDP Listeners ==="
ss -unlp
echo ""

echo "=== 7. TCP Stack Parameters ==="
echo "  tcp_rmem (min default max): $(cat /proc/sys/net/ipv4/tcp_rmem)"
echo "  tcp_wmem (min default max): $(cat /proc/sys/net/ipv4/tcp_wmem)"
echo "  tcp_congestion:             $(cat /proc/sys/net/ipv4/tcp_congestion_control)"
echo "  tcp_max_syn_backlog:        $(cat /proc/sys/net/ipv4/tcp_max_syn_backlog)"
echo "  tcp_fin_timeout:            $(cat /proc/sys/net/ipv4/tcp_fin_timeout)"
echo "  tcp_keepalive_time:         $(cat /proc/sys/net/ipv4/tcp_keepalive_time)"
echo "  tcp_keepalive_intvl:        $(cat /proc/sys/net/ipv4/tcp_keepalive_intvl)"
echo "  tcp_keepalive_probes:       $(cat /proc/sys/net/ipv4/tcp_keepalive_probes)"
echo ""

echo "=== 8. Network Buffer Sizes ==="
echo "  rmem_default: $(cat /proc/sys/net/core/rmem_default)"
echo "  rmem_max:     $(cat /proc/sys/net/core/rmem_max)"
echo "  wmem_default: $(cat /proc/sys/net/core/wmem_default)"
echo "  wmem_max:     $(cat /proc/sys/net/core/wmem_max)"
echo ""

echo "=== 9. IP Statistics ==="
cat /proc/net/snmp | grep -E "^(Ip|Tcp|Udp|Icmp):" | head -8
echo ""

echo "=== 10. Connection Tracking ==="
if [ -f /proc/sys/net/netfilter/nf_conntrack_count ]; then
    echo "  Current connections: $(cat /proc/sys/net/netfilter/nf_conntrack_count)"
    echo "  Max connections:     $(cat /proc/sys/net/netfilter/nf_conntrack_max)"
else
    echo "  (conntrack not loaded)"
fi
echo ""

echo "=== 11. Available Network Kernel Functions ==="
if [ -d /sys/kernel/tracing ]; then
    echo "  Network-related traceable functions:"
    cat /sys/kernel/tracing/available_filter_functions 2>/dev/null | \
        grep -E "^(ip_rcv|tcp_v4|udp_rcv|arp_rcv|icmp_rcv|netif_receive|dev_queue_xmit)" | \
        head -20
elif [ -d /sys/kernel/debug/tracing ]; then
    echo "  Network-related traceable functions:"
    cat /sys/kernel/debug/tracing/available_filter_functions 2>/dev/null | \
        grep -E "^(ip_rcv|tcp_v4|udp_rcv|arp_rcv|icmp_rcv|netif_receive|dev_queue_xmit)" | \
        head -20
else
    echo "  (ftrace not available)"
fi
echo ""

echo "=== 12. Network Device Driver Info ==="
for iface in $(ls /sys/class/net/); do
    echo "  $iface:"
    driver=$(readlink /sys/class/net/$iface/device/driver 2>/dev/null | xargs basename 2>/dev/null || echo "n/a")
    echo "    Driver: $driver"
    echo "    MAC:    $(cat /sys/class/net/$iface/address 2>/dev/null || echo 'n/a')"
    echo "    MTU:    $(cat /sys/class/net/$iface/mtu 2>/dev/null || echo 'n/a')"
    echo "    State:  $(cat /sys/class/net/$iface/operstate 2>/dev/null || echo 'n/a')"
done
echo ""

echo "=== 13. Ftrace: Trace TCP connect (quick demo) ==="
TRACE_DIR=""
if [ -d /sys/kernel/tracing ]; then
    TRACE_DIR="/sys/kernel/tracing"
elif [ -d /sys/kernel/debug/tracing ]; then
    TRACE_DIR="/sys/kernel/debug/tracing"
fi

if [ -n "$TRACE_DIR" ]; then
    echo 0 > $TRACE_DIR/tracing_on
    echo nop > $TRACE_DIR/current_tracer
    echo > $TRACE_DIR/set_ftrace_filter
    echo 'tcp_v4_connect' > $TRACE_DIR/set_ftrace_filter
    echo 'tcp_connect' >> $TRACE_DIR/set_ftrace_filter
    echo function > $TRACE_DIR/current_tracer
    echo 1 > $TRACE_DIR/tracing_on

    # Generate a TCP connection
    curl -s --max-time 2 http://example.com > /dev/null 2>&1 || true

    echo 0 > $TRACE_DIR/tracing_on
    echo "  Trace output:"
    cat $TRACE_DIR/trace | grep -v "^#" | head -10
    echo nop > $TRACE_DIR/current_tracer
    echo > $TRACE_DIR/set_ftrace_filter
else
    echo "  (ftrace not available)"
fi
echo ""

echo "Exploration complete!"
