# Mandeye TEC

## Overview

Mandeye TEC couples a high‑performance C++ core with a lightweight Flask
web interface to control and monitor a Livox lidar and GNSS receiver. The
core application exposes control and status information through a shared
library, while the web layer provides a browser‑based dashboard and a REST
API for remote control.

### Architecture

* **C++ core** – located in the `code/` directory and built into
  `libmandeye_core.so`. It manages lidar and GNSS devices, saves LAZ files
  and publishes status updates over ZeroMQ.
* **Flask web interface** – found in the `web/` directory. It loads the
  shared library via `ctypes` and offers API endpoints and a simple Vue
  based front‑end for starting/stopping scans and viewing status.

## Dependencies and requirements

### Software

The project relies on the following packages (installed by
`scripts/install_dependencies.sh`):

* `build-essential`, `cmake`
* `libserial-dev`, `libzmq3-dev`
* `python3`, `python3-pip`
* Python packages listed in `requirements.txt` (`Flask`, `pyzmq`)

### Hardware

* A computer running Linux with network access to a **Livox lidar**
  (e.g. MID360 or HAP).
* A **GNSS receiver** connected via serial port.
* USB storage or other repository path for saving scans.

## Installation

1. Install system and Python dependencies:

   ```bash
   scripts/install_dependencies.sh
   ```

2. Compile and install the Livox SDK2 (required before building):

   ```bash
   git submodule update --init --recursive 3rd/Livox-SDK2
   cd 3rd/Livox-SDK2
   mkdir build && cd build
   cmake .. && make -j
   sudo make install  # installs headers and libs to /usr/local
   ```

   If you install to a non-standard prefix, set `LIVOX_SDK_DIR` to that
   location before running the build script.

3. Build the C++ core:

   ```bash
   scripts/build.sh
   ```

4. Edit configuration settings in
   [`config/mandeye_config.json`](config/mandeye_config.json) to match your
   environment (see **Configuration** below).

5. Launch the web interface:

   ```bash
   scripts/run_web.sh
   ```

   The UI is available on `http://localhost:5000` unless overridden by the
   `WEB_PORT` environment variable or configuration file.

## Configuration

Runtime settings are loaded from `config/mandeye_config.json` at startup. The
file supports the following fields:

* `livox_interface_ip` – Livox interface IP address (default
  `"192.168.1.5"`).
* `repository_path` – Path to USB repository (default `"/media/usb/"`).
* `server_port` – Port used by the C++ publisher (default `8003`).
* `web_port` – Port for the Flask web UI (default `5000`).

For each setting the application uses the following precedence:

1. Value from the configuration file
2. Environment variable override (`MANDEYE_LIVOX_LISTEN_IP`,
   `MANDEYE_REPO`, `SERVER_PORT`, `WEB_PORT`)
3. Compiled default embedded in the binaries

Modify the configuration file or set environment variables as needed before
starting the application. After making changes, restart any running
instances for the new settings to take effect.

## Common operations

### Starting and stopping scans

From the web dashboard use the **Start Scan**, **Stop Scan** or
**Trigger Stop Scan** buttons. The same actions are available via the REST
API:

```bash
curl -X POST http://localhost:5000/api/start_scan   # start
curl -X POST http://localhost:5000/api/stop_scan    # stop
curl -X POST http://localhost:5000/api/stopscan     # emergency stop
```

### Monitoring status

The dashboard shows live lidar, GNSS and filesystem information. Status can
also be queried directly:

```bash
curl http://localhost:5000/api/status       # summary
curl http://localhost:5000/api/status_full  # detailed
```

## Troubleshooting

* **Build errors** – ensure all dependencies are installed and required
  third‑party sources (e.g. LASzip) are present. Running
  `scripts/install_dependencies.sh` and fetching git submodules usually
  resolves missing headers.
* **`libmandeye_core.so not found`** – run `scripts/build.sh` before
  executing `scripts/run_web.sh`.
* **Device connectivity issues** – verify network settings for the lidar
  and serial port permissions for the GNSS receiver.
* **No status updates** – check that the publisher port (`server_port`) is
  open and not blocked by a firewall.

Edit the configuration file or set environment variables as needed before
starting the application.

## Docker

The project can be built and run in a container:

```bash
scripts/build_docker.sh
scripts/run_docker.sh
```

The run script exposes the web interface on port `5000` and mounts the
`config` directory into the container so you can edit `config/mandeye_config.json`
on the host and have those settings applied inside the container.

## Logging

The core application emits severity-tagged logs to `logs/mandeye.log`. The file
is rotated when it exceeds 5&nbsp;MB, keeping a single backup `mandeye.log.1`.
Recent log lines can be streamed via the `/api/logs` endpoint and are displayed
in the web interface with level filters for info, warnings, and errors.

