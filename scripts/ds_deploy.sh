#!/usr/bin/env bash
set -euo pipefail

TARGET_DIR="${1:-/opt/mandeye}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."
BUILD_DIR="${ROOT_DIR}/build"
CONFIG_DIR="${ROOT_DIR}/config"

if [ ! -d "$BUILD_DIR" ]; then
  echo "Build directory not found. Run scripts/ds_build_app.sh first." >&2
  exit 1
fi

sudo mkdir -p "$TARGET_DIR"
sudo cp -r "$BUILD_DIR" "$TARGET_DIR/"
sudo cp -r "$CONFIG_DIR" "$TARGET_DIR/"

echo "Deployed build and config to $TARGET_DIR"
