#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."
BUILD_DIR="${ROOT_DIR}/build"
LIB_FILE="${BUILD_DIR}/libmandeye_core.so"
VENV_DIR="${ROOT_DIR}/.venv"

if [ ! -f "$LIB_FILE" ]; then
  echo "Error: $LIB_FILE not found. Please build the project first." >&2
  exit 1
fi

if [ ! -d "$VENV_DIR" ]; then
  python3 -m venv "$VENV_DIR"
  "$VENV_DIR/bin/pip" install -r "${ROOT_DIR}/requirements.txt"
fi

WEB_PORT="${WEB_PORT:-5000}"
export WEB_PORT

cd "${ROOT_DIR}/web"
"${VENV_DIR}/bin/flask" --app app run --host=0.0.0.0 --port "$WEB_PORT"
