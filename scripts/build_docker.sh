#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."

IMAGE_NAME="mandeye_tec:latest"

docker build -t "$IMAGE_NAME" "$ROOT_DIR"
