#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

"${SCRIPT_DIR}/install_dependencies.sh"
"${SCRIPT_DIR}/setup_service.sh"
"${SCRIPT_DIR}/setup_usb_mount.sh"

echo "Installation complete. Please reboot the system for USB automount configuration to take effect."