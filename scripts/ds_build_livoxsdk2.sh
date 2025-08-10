#!/usr/bin/env bash
# Build the Livox-SDK2 library.
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/3rd/Livox-SDK2/build"

if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
  echo "Building Livox-SDK2..."
  cmake -S "$PROJECT_ROOT/3rd/Livox-SDK2" -B "$BUILD_DIR"
  cmake --build "$BUILD_DIR" --config Release
  SUDO=""
  if command -v sudo >/dev/null 2>&1; then
    SUDO="sudo"
  fi
  $SUDO cmake --install "$BUILD_DIR"
else
  echo "Livox-SDK2 already built."
fi

