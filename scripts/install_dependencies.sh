#!/usr/bin/env bash
set -euo pipefail

sudo apt-get update
sudo apt-get install -y build-essential cmake libserial-dev libzmq3-dev python3 python3-pip
