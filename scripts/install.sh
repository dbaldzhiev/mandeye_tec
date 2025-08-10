#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

"${SCRIPT_DIR}/install_dependencies.sh"
"${SCRIPT_DIR}/setup_service.sh"

echo "Install complete. Enable the service with: sudo systemctl enable --now mandeye.service"
