#!/usr/bin/env bash
set -euo pipefail

FORCE=false
TARGET_DIR="/opt/mandeye"
while [[ $# -gt 0 ]]; do
  case "$1" in
    --force|-f)
      FORCE=true
      shift
      ;;
    *)
      TARGET_DIR="$1"
      shift
      ;;
  esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."
BUILD_DIR="${ROOT_DIR}/build"
CONFIG_DIR="${ROOT_DIR}/config"

if [ ! -d "$BUILD_DIR" ]; then
  echo "Build directory not found. Run scripts/ds_build_app.sh first." >&2
  exit 1
fi

sudo mkdir -p "$TARGET_DIR"

# Check if deployment is up to date
build_changes=$(rsync -rcn --delete --out-format='%n' "$BUILD_DIR/" "$TARGET_DIR/build/" || true)
config_changes=$(rsync -rcn --delete --out-format='%n' "$CONFIG_DIR/" "$TARGET_DIR/config/" || true)
if [[ -z "$build_changes$config_changes" && $FORCE == false ]]; then
  echo "Target already up to date. Use --force to redeploy."
  exit 0
fi

sudo rsync -a --delete "$BUILD_DIR/" "$TARGET_DIR/build/"
sudo rsync -a --delete "$CONFIG_DIR/" "$TARGET_DIR/config/"

echo "Deployed build and config to $TARGET_DIR"

