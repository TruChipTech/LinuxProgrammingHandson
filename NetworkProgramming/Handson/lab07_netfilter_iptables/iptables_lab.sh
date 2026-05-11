#!/bin/bash
# iptables_lab.sh — iptables hands-on exercises
#
# Run: sudo bash iptables_lab.sh
#
# WARNING: This script modifies iptables rules. Use in a VM or test environment.

set -e

echo "========================================="
echo "  iptables Lab Exercises"
echo "========================================="
echo ""

# Check root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root: sudo bash $0"
    exit 1
fi

echo "=== Current iptables rules ==="
iptables -L -n -v --line-numbers
echo ""

echo "=== Exercise 1: Save current rules ==="
iptables-save > /tmp/iptables_backup.rules
echo "Rules saved to /tmp/iptables_backup.rules"
echo ""

echo "=== Exercise 2: Block incoming ICMP (ping) ==="
iptables -A INPUT -p icmp -j DROP
echo "Rule added. Try: ping -c 2 localhost (should timeout)"
echo "Press Enter to continue..."
read -r

echo "=== Exercise 3: Remove the ICMP block ==="
iptables -D INPUT -p icmp -j DROP
echo "Rule removed. Ping should work again."
echo ""

echo "=== Exercise 4: Log and drop packets to port 12345 ==="
iptables -A INPUT -p tcp --dport 12345 -j LOG --log-prefix "BLOCKED_12345: "
iptables -A INPUT -p tcp --dport 12345 -j DROP
echo "Rules added. Try: nc -zv localhost 12345"
echo "Check: dmesg | grep BLOCKED_12345"
echo "Press Enter to continue..."
read -r

echo "=== Cleanup ==="
iptables -D INPUT -p tcp --dport 12345 -j LOG --log-prefix "BLOCKED_12345: " 2>/dev/null || true
iptables -D INPUT -p tcp --dport 12345 -j DROP 2>/dev/null || true
echo "Rules cleaned up."
echo ""

echo "=== Exercise 5: Allow only SSH and block everything else ==="
echo "Example commands (NOT executed — would lock you out!):"
echo "  iptables -P INPUT DROP"
echo "  iptables -A INPUT -p tcp --dport 22 -j ACCEPT"
echo "  iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT"
echo "  iptables -A INPUT -i lo -j ACCEPT"
echo ""

echo "=== Exercise 6: NAT / Port forwarding ==="
echo "Example commands (NOT executed):"
echo "  # Enable IP forwarding"
echo "  sysctl -w net.ipv4.ip_forward=1"
echo ""
echo "  # Masquerade (internet sharing)"
echo "  iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE"
echo ""
echo "  # Port forward: external:8080 → internal:192.168.1.100:80"
echo "  iptables -t nat -A PREROUTING -p tcp --dport 8080 -j DNAT --to 192.168.1.100:80"
echo ""

echo "=== Restore original rules ==="
iptables-restore < /tmp/iptables_backup.rules
echo "Original rules restored."
echo ""
echo "Lab complete!"
