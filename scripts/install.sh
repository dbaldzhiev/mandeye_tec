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

non_interactive=false
while [[ $# -gt 0 ]]; do
  case "$1" in
    --yes|-y)
      non_interactive=true
      shift
      ;;
    *)
      echo "Usage: $0 [--yes]" >&2
      exit 1
      ;;
  esac
done

for step in "${steps[@]}"; do
  desc=${step%%|*}
  script=${step##*|}
  if $non_interactive; then
    ans="y"
  else
    read -rp "Run step: $desc? [y/N] " ans
  fi
  if [[ "$ans" =~ ^[Yy]$ ]]; then
    "${SCRIPT_DIR}/${script}"
  else
    echo "Skipping $desc"
  fi
  echo
done

echo "Installation complete. Please reboot if system components were installed."
