#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."
CONFIG_FILE="${ROOT_DIR}/config/mandeye_config.json"
IP="192.168.6.1"
if command -v jq >/dev/null 2>&1 && [[ -f "$CONFIG_FILE" ]]; then
  IP=$(jq -r '.livox_interface_ip // "192.168.6.1"' "$CONFIG_FILE")
fi
NETPLAN_FILE="/etc/netplan/99-mandeye-eth0.yaml"
if [[ ! -f "$NETPLAN_FILE" ]]; then
  sudo tee "$NETPLAN_FILE" > /dev/null <<EOF2
network:
  version: 2
  renderer: networkd
  ethernets:
    eth0:
      dhcp4: no
      addresses: [$IP/24]
EOF2
  sudo netplan apply
  echo "Static IP $IP configured for eth0"
else
  echo "Netplan configuration already exists at $NETPLAN_FILE"
fi
