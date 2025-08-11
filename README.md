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
`scripts/install.sh`):

* `build-essential`, `cmake`
* `libserial-dev`, `libzmq3-dev`
* `python3`, `python3-pip`, `python3-venv`
* `jq`
* Python packages listed in `requirements.txt` (`Flask`, `pyzmq`)

### Hardware

* A computer running Linux with network access to a **Livox lidar**
  (e.g. MID360 or HAP).
* A **GNSS receiver** connected via serial port.
* USB storage or other repository path for saving scans.

## Installation

1. Run the interactive installer and follow the prompts:

   ```bash
   scripts/install.sh
   ```

   The script pulls git submodules, installs required packages, prepares a
   Python virtual environment, configures networking and services, compiles
   third‑party libraries, and builds the application. Reboot after completion
   to apply any USB automount or network changes.

2. (Optional) Start the systemd service immediately:

   ```bash
   sudo systemctl start mandeye.service
   ```

   Disable or stop the service with:

   ```bash
   sudo systemctl disable --now mandeye.service
   ```

3. Launch the web interface manually (useful for development):

   ```bash
   scripts/run_web.sh
   ```

   The UI is available on `http://localhost:5000`.

Outputs are stored in `/media/usb`, the web interface listens on port `5000`,
and the Livox lidar interface IP is automatically determined from `eth0`.

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
  `scripts/install.sh` and fetching git submodules usually resolves
  missing headers.
* **`libmandeye_core.so not found`** – run `scripts/ds_build_app.sh` before
  executing `scripts/run_web.sh`.
* **Device connectivity issues** – verify network settings for the lidar
  and serial port permissions for the GNSS receiver.
* **No status updates** – check that the publisher port (`server_port`) is
  open and not blocked by a firewall.


## Docker

The project can be built and run in a container:

```bash
scripts/build_docker.sh
scripts/run_docker.sh
```

The run script exposes the web interface on port `5000`.

## Logging

The core application emits severity-tagged logs to `logs/mandeye.log`. The file
is rotated when it exceeds 5&nbsp;MB, keeping a single backup `mandeye.log.1`.
Recent log lines can be streamed via the `/api/logs` endpoint and are displayed
in the web interface with level filters for info, warnings, and errors.

