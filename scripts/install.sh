#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

steps=(
  "Pull git submodules|ds_pull_submodules.sh"
  "Install system dependencies|ds_install_dependencies.sh"
  "Create Python virtual environment|ds_setup_venv.sh"
  "Configure static IP for eth0|ds_setup_static_ip.sh"
  "Configure USB automount|ds_setup_usb_mount.sh"
  "Install webapp systemd service|ds_setup_service.sh"
  "Compile Livox SDK2|ds_build_livoxsdk2.sh"
  "Build main application|ds_build_app.sh"
)

for step in "${steps[@]}"; do
  desc=${step%%|*}
  script=${step##*|}
  read -rp "Run step: $desc? [y/N] " ans
  if [[ "$ans" =~ ^[Yy]$ ]]; then
    "${SCRIPT_DIR}/${script}"
  else
    echo "Skipping $desc"
  fi
  echo
  read -rp "Press Enter to continue..." _
  echo

done

echo "Installation complete. Please reboot if system components were installed."
