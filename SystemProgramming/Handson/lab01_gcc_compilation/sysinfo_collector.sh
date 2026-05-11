#!/bin/bash
# sysinfo_collector.sh — System Information Report Generator
#
# Exercise: This is a starter template. Run it and verify the output.
# Then extend it with additional fields (see TODOs).
#
# Usage: chmod +x sysinfo_collector.sh && ./sysinfo_collector.sh

echo "=========================================="
echo "        SYSTEM INFORMATION REPORT         "
echo "=========================================="
echo "Report Date:    $(date)"
echo ""

# Basic system info
echo "Hostname:       $(hostname)"
echo "Kernel:         $(uname -r)"
echo "Architecture:   $(uname -m)"
echo "OS:             $(cat /etc/os-release 2>/dev/null | grep PRETTY_NAME | cut -d'"' -f2)"
echo "CPUs:           $(nproc)"

# Memory
MEM_KB=$(grep MemTotal /proc/meminfo | awk '{print $2}')
MEM_MB=$((MEM_KB / 1024))
echo "Total RAM:      ${MEM_MB} MB"

# Uptime
UPTIME_SEC=$(cat /proc/uptime | awk '{print $1}' | cut -d. -f1)
UPTIME_DAYS=$((UPTIME_SEC / 86400))
UPTIME_HRS=$(((UPTIME_SEC % 86400) / 3600))
echo "Uptime:         ${UPTIME_DAYS} days, ${UPTIME_HRS} hours"

# Load
echo "Load Average:   $(cat /proc/loadavg | awk '{print $1, $2, $3}')"

# Filesystem
echo "Root FS Type:   $(df -T / | tail -1 | awk '{print $2}')"
echo "Root FS Usage:  $(df -h / | tail -1 | awk '{print $5}')"

# TODO: Add these additional fields:
# - Total disk space
# - Number of running processes
# - Currently logged-in users
# - Default gateway IP
# - DNS server(s)
# - GCC version
# - Python version
# - Last boot time

echo ""
echo "=========================================="
echo "          END OF REPORT                   "
echo "=========================================="
