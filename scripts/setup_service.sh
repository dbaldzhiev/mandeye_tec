#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."
SERVICE_FILE="/etc/systemd/system/mandeye.service"

if command -v jq >/dev/null 2>&1; then
  WEB_PORT="$(jq -r '.web_port // 5000' "${ROOT_DIR}/config/mandeye_config.json" 2>/dev/null)"
else
  WEB_PORT="5000"
fi

PYTHON_BIN="$(command -v python3 || command -v python)"

sudo tee "$SERVICE_FILE" > /dev/null <<SERVICE
[Unit]
Description=Mandeye Flask Service
After=network.target

[Service]
Type=simple
WorkingDirectory=${ROOT_DIR}
ExecStart=${PYTHON_BIN} -m flask run --host=0.0.0.0 --port=${WEB_PORT}
Restart=on-failure
Environment=PYTHONPATH=${ROOT_DIR}
Environment=FLASK_APP=web/app.py

[Install]
WantedBy=multi-user.target
SERVICE

sudo systemctl daemon-reload

echo "Service installed to $SERVICE_FILE"
