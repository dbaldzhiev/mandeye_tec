#!/usr/bin/env bash
set -euo pipefail

# Install usbmount and exFAT support
sudo apt-get update
sudo apt-get install -y usbmount exfat-fuse exfat-utils

USBMOUNT_CONF=/etc/usbmount/usbmount.conf
if [[ -f "$USBMOUNT_CONF" ]]; then
  # Configure mount options and supported filesystems
  sudo sed -i 's|^MOUNTOPTIONS=.*|MOUNTOPTIONS="sync,noexec,nodev,noatime,uid=1000,gid=1000"|' "$USBMOUNT_CONF"
  sudo sed -i 's|^FILESYSTEMS=.*|FILESYSTEMS="vfat ext2 ext3 ext4 hfsplus ntfs fuseblk exfat"|' "$USBMOUNT_CONF"
else
  echo "usbmount configuration file not found at $USBMOUNT_CONF" >&2
  exit 1
fi

# Ensure usbmount works with systemd's PrivateMounts
sudo mkdir -p /etc/systemd/system/systemd-udevd.service.d
sudo tee /etc/systemd/system/systemd-udevd.service.d/usbmount.conf > /dev/null <<'EOC'
[Service]
PrivateMounts=no
EOC

sudo systemctl daemon-reload

echo "USB automount configured. Reboot required to apply changes."
