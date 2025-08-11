#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."

IMAGE_NAME="mandeye_tec:latest"

# Map web port
WEB_PORT="5000"

docker run --rm -it -p "$WEB_PORT:$WEB_PORT" "$IMAGE_NAME"
