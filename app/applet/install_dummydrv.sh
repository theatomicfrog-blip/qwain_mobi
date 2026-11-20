#!/bin/bash
# ==============================================================================
# install_dummydrv.sh - Automated Installation Script for dummydrv CUPS Backend
# ==============================================================================

# Ensure script is run as root
if [ "$EUID" -ne 0 ]; then
    echo "[-] Error: Please run this installation script as root (sudo)."
    exit 1
fi

echo "[+] Starting dummydrv CUPS installation..."

# 1. Install CUPS if not already present
if ! command -v lpadmin >/dev/null 2>&1; then
    echo "[*] CUPS is not installed. Installing CUPS package..."
    apt-get update && apt-get install -y cups file
else
    echo "[+] CUPS is already installed."
fi

# 2. Copy the backend into the CUPS backend directory
CUPS_BACKEND_DIR="/usr/lib/cups/backend"
if [ ! -d "$CUPS_BACKEND_DIR" ]; then
    echo "[-] Error: CUPS backend directory ($CUPS_BACKEND_DIR) does not exist."
    exit 1
fi

echo "[*] Copying backend to $CUPS_BACKEND_DIR/dummydrv..."
cp dummydrv "$CUPS_BACKEND_DIR/dummydrv"

# 3. Set strict CUPS backend permissions
# CUPS backends must be owned by root, and 755/700 depending on environment.
# 700 allows root-only backend execution, 755 allows discovery.
echo "[*] Setting execution permissions..."
chown root:root "$CUPS_BACKEND_DIR/dummydrv"
chmod 755 "$CUPS_BACKEND_DIR/dummydrv"

# 4. Restart CUPS to discover the new backend
echo "[*] Restarting CUPS service..."
if command -v systemctl >/dev/null 2>&1; then
    systemctl restart cups
elif command -v service >/dev/null 2>&1; then
    service cups restart
else
    /etc/init.d/cups restart
fi

# Wait brief moment for CUPS to reinitialize
sleep 1

# 5. Verify CUPS detects the new backend
echo "[*] Verifying backend registration..."
if lpinfo -v | grep -q "dummydrv"; then
    echo "[+] dummydrv registered successfully!"
else
    echo "[-] Warning: CUPS did not return dummydrv on lpinfo query."
fi

# 6. Delete any existing Dummy_Printer and add a clean one
echo "[*] Configuring printer Dummy_Printer in CUPS..."
lpadmin -x Dummy_Printer 2>/dev/null
lpadmin -p Dummy_Printer -v dummydrv:// -E -m raw

# 7. Enable the printer and accept print jobs
cupsaccept Dummy_Printer 2>/dev/null
cupsenable Dummy_Printer 2>/dev/null

echo "======================================================================"
echo "[+] dummydrv installation and configuration complete!"
echo "======================================================================"
echo "- Printer Name: Dummy_Printer"
echo "- Backend Path: $CUPS_BACKEND_DIR/dummydrv"
echo "- Analysis Log: /tmp/dummydrv/dummydrv_analysis.log"
echo "- Job Payloads: /tmp/dummydrv/job_<id>_data.bin"
echo "======================================================================"
