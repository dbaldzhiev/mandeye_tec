#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."
SERVICE_FILE="/etc/systemd/system/mandeye.service"

if [[ -f "$SERVICE_FILE" ]]; then
  echo "Service already configured at $SERVICE_FILE"
  exit 0
fi

WEB_PORT="5000"

PYTHON_BIN="${ROOT_DIR}/.venv/bin/python"
if [[ ! -x "$PYTHON_BIN" ]]; then
  PYTHON_BIN="$(command -v python3 || command -v python)"
fi

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
sudo systemctl enable mandeye.service

echo "Service installed to $SERVICE_FILE"
