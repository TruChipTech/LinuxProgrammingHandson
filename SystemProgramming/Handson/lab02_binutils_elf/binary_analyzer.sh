#!/bin/bash
# binary_analyzer.sh — Automated Binary Analysis Tool
#
# Usage: ./binary_analyzer.sh <binary_file>
#
# Generates a comprehensive analysis report of any ELF binary.
# Exercise: Fill in the TODO sections to complete the analyzer.

set -euo pipefail

# Color codes for pretty output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

BINARY="$1"
REPORT_FILE="analysis_report_$(basename "$BINARY").txt"

if [ $# -ne 1 ]; then
    echo "Usage: $0 <binary_file>"
    exit 1
fi

if [ ! -f "$BINARY" ]; then
    echo "Error: File '$BINARY' not found!"
    exit 1
fi

# ============================================================
# Helper Functions
# ============================================================

print_header() {
    echo -e "${CYAN}╔══════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║${NC} ${GREEN}$1${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════════════════╝${NC}"
}

section() {
    echo ""
    echo "=================================================================="
    echo " $1"
    echo "=================================================================="
}

# ============================================================
# Start Report
# ============================================================

{
    echo "╔══════════════════════════════════════════════════════════════╗"
    echo "║           BINARY ANALYSIS REPORT                           ║"
    echo "║           Generated: $(date)              ║"
    echo "╚══════════════════════════════════════════════════════════════╝"
    echo ""
    echo "Target: $BINARY"
    echo "MD5:    $(md5sum "$BINARY" | awk '{print $1}')"
    echo "SHA256: $(sha256sum "$BINARY" | awk '{print $1}')"
    echo "Size:   $(stat -c %s "$BINARY") bytes"

    # ----------------------------------------------------------
    section "1. FILE IDENTIFICATION"
    # ----------------------------------------------------------
    file "$BINARY"

    # ----------------------------------------------------------
    section "2. ELF HEADER"
    # ----------------------------------------------------------
    readelf -h "$BINARY" 2>/dev/null || echo "Not an ELF binary"

    # ----------------------------------------------------------
    section "3. SECTION HEADERS"
    # ----------------------------------------------------------
    readelf -S "$BINARY" 2>/dev/null || echo "No section headers"

    # ----------------------------------------------------------
    section "4. SECTION SIZES"
    # ----------------------------------------------------------
    size "$BINARY" 2>/dev/null || echo "Cannot determine sizes"

    # ----------------------------------------------------------
    section "5. PROGRAM HEADERS (Segments)"
    # ----------------------------------------------------------
    readelf -l "$BINARY" 2>/dev/null || echo "No program headers"

    # ----------------------------------------------------------
    section "6. SYMBOLS"
    # ----------------------------------------------------------
    echo "--- Symbol count ---"
    nm "$BINARY" 2>/dev/null | wc -l || echo "0 (stripped?)"
    echo ""
    echo "--- Text (code) symbols ---"
    nm "$BINARY" 2>/dev/null | grep " T \| t " | head -20 || echo "None found"
    echo ""
    echo "--- Data symbols ---"
    nm "$BINARY" 2>/dev/null | grep " D \| d \| B \| b " | head -20 || echo "None found"

    # ----------------------------------------------------------
    section "7. SHARED LIBRARY DEPENDENCIES"
    # ----------------------------------------------------------
    ldd "$BINARY" 2>/dev/null || echo "Not a dynamic executable"

    # ----------------------------------------------------------
    section "8. DYNAMIC SECTION"
    # ----------------------------------------------------------
    readelf -d "$BINARY" 2>/dev/null | head -30 || echo "No dynamic section"

    # ----------------------------------------------------------
    section "9. INTERESTING STRINGS"
    # ----------------------------------------------------------
    echo "--- Potential passwords/keys/secrets ---"
    strings "$BINARY" | grep -i "password\|secret\|key\|token\|flag\|crypt" || echo "None found"
    echo ""
    echo "--- Potential file paths ---"
    strings "$BINARY" | grep -E "^/" | head -20 || echo "None found"
    echo ""
    echo "--- Potential URLs/IPs ---"
    # TODO: Add regex to find URLs and IP addresses
    strings "$BINARY" | grep -E "https?://|[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+" || echo "None found"
    echo ""
    echo "--- All unique strings (first 50) ---"
    strings "$BINARY" | sort -u | head -50

    # ----------------------------------------------------------
    section "10. DISASSEMBLY HIGHLIGHTS"
    # ----------------------------------------------------------
    echo "--- Entry point ---"
    objdump -d "$BINARY" 2>/dev/null | grep -A 15 "<_start>\|<main>" | head -40 || echo "Cannot disassemble"
    echo ""
    echo "--- Function calls ---"
    objdump -d "$BINARY" 2>/dev/null | grep "call" | sort -u | head -30 || echo "No calls found"

    # ----------------------------------------------------------
    section "11. SECURITY ANALYSIS"
    # ----------------------------------------------------------
    echo "--- Dangerous function imports ---"
    # TODO: Check for dangerous functions (gets, strcpy, sprintf, system, exec*)
    nm -D "$BINARY" 2>/dev/null | grep -E "gets|strcpy|sprintf|system|exec" || echo "None found (good!)"
    echo ""
    echo "--- Stack protection ---"
    readelf -s "$BINARY" 2>/dev/null | grep "stack_chk" && echo "Stack canary: ENABLED" || echo "Stack canary: DISABLED"
    echo ""
    echo "--- RELRO ---"
    readelf -l "$BINARY" 2>/dev/null | grep "GNU_RELRO" && echo "RELRO: ENABLED" || echo "RELRO: DISABLED"
    echo ""
    echo "--- NX (No Execute) ---"
    readelf -l "$BINARY" 2>/dev/null | grep "GNU_STACK" || echo "Cannot determine"

    # TODO Exercise: Add these checks:
    # 1. Check if binary is packed (look for UPX! string)
    # 2. Check for PIE (Position Independent Executable)
    # 3. Count total unique syscalls referenced

    # ----------------------------------------------------------
    section "12. RISK ASSESSMENT"
    # ----------------------------------------------------------
    RISK="LOW"
    REASONS=""
    
    if strings "$BINARY" | grep -qi "exec\|system\|popen"; then
        RISK="MEDIUM"
        REASONS="${REASONS}\n  - Contains exec/system calls"
    fi
    if strings "$BINARY" | grep -qi "socket\|connect\|bind"; then
        RISK="MEDIUM"
        REASONS="${REASONS}\n  - Contains network operations"
    fi
    if strings "$BINARY" | grep -qi "ptrace\|anti.debug"; then
        RISK="HIGH"
        REASONS="${REASONS}\n  - Contains anti-debugging techniques"
    fi
    
    echo "Risk Level: $RISK"
    if [ -n "$REASONS" ]; then
        echo -e "Reasons:$REASONS"
    fi

    echo ""
    echo "=================================================================="
    echo " END OF REPORT"
    echo "=================================================================="

} | tee "$REPORT_FILE"

echo -e "\n${GREEN}Report saved to: ${REPORT_FILE}${NC}"
