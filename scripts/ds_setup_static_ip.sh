#!/usr/bin/env bash
set -euo pipefail

# Configure a persistent static IP of 192.168.6.1/24 on eth0.
# Supports systems using netplan, systemd-networkd, or dhcpcd. The script
# is idempotent and will not duplicate configuration if re-run.

if command -v netplan >/dev/null 2>&1; then
  CONFIG_FILE="/etc/netplan/10-eth0-static.yaml"
  if [[ ! -f "$CONFIG_FILE" ]]; then
    echo "Creating netplan configuration at $CONFIG_FILE"
    sudo tee "$CONFIG_FILE" >/dev/null <<'EOF'
network:
  version: 2
  renderer: networkd
  ethernets:
    eth0:
      dhcp4: no
      addresses: [192.168.6.1/24]
EOF
    sudo netplan apply
  else
    echo "Netplan configuration already exists at $CONFIG_FILE"
  fi
elif systemctl list-unit-files 2>/dev/null | grep -q systemd-networkd; then
  CONFIG_FILE="/etc/systemd/network/10-eth0.network"
  if [[ ! -f "$CONFIG_FILE" ]]; then
    echo "Creating systemd-networkd configuration at $CONFIG_FILE"
    sudo tee "$CONFIG_FILE" >/dev/null <<'EOF'
[Match]
Name=eth0

[Network]
Address=192.168.6.1/24
EOF
    sudo systemctl restart systemd-networkd
  else
    echo "systemd-networkd configuration already exists at $CONFIG_FILE"
  fi
elif command -v dhcpcd >/dev/null 2>&1; then
  CONFIG_FILE="/etc/dhcpcd.conf"
  if ! sudo grep -q "^interface eth0" "$CONFIG_FILE"; then
    echo "Appending dhcpcd configuration to $CONFIG_FILE"
    sudo tee -a "$CONFIG_FILE" >/dev/null <<'EOF'
interface eth0
static ip_address=192.168.6.1/24
nohook wpa_supplicant
EOF
    sudo systemctl restart dhcpcd
  else
    echo "dhcpcd configuration for eth0 already present in $CONFIG_FILE"
  fi
else
  echo "No supported network configuration system found (netplan, systemd-networkd, dhcpcd)" >&2
  exit 1
fi

echo "Static IP configuration complete."
