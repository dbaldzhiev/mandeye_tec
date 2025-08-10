#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."
cd "$ROOT_DIR"
if git submodule status | grep -q '^-' ; then
  echo "Initializing and updating git submodules..."
  git submodule update --init --recursive
else
  echo "Git submodules already initialized."
fi
