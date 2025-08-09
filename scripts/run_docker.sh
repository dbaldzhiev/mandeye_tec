#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."

IMAGE_NAME="mandeye_tec:latest"
CONFIG_DIR="${ROOT_DIR}/config"

# Map web port and configuration directory
WEB_PORT="${WEB_PORT:-5000}"

docker run --rm -it -p "$WEB_PORT:$WEB_PORT" -v "${CONFIG_DIR}:/app/config" "$IMAGE_NAME"
