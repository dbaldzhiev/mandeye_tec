#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."
BUILD_DIR="${ROOT_DIR}/build"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
if [[ -n "${LIVOX_SDK_DIR:-}" ]]; then
  if [[ ! -f "${LIVOX_SDK_DIR}/lib/liblivox_lidar_sdk_static.a" ]]; then
    echo "Livox SDK not found in ${LIVOX_SDK_DIR}." >&2
    echo "Please compile Livox SDK2 and set LIVOX_SDK_DIR to its installation path." >&2
    exit 1
  fi
  cmake -DLIVOX_SDK_DIR="${LIVOX_SDK_DIR}" ..
else
  cmake ..
fi
make
