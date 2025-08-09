# Mandeye TEC

## Configuration

Runtime settings are loaded from `config/mandeye_config.json` at startup.
The file supports the following fields:

- `livox_interface_ip` – Livox interface IP address (default `"192.168.1.5"`).
- `repository_path` – Path to USB repository (default `"/media/usb/"`).
- `server_port` – Port used by the C++ publisher (default `8003`).
- `web_port` – Port for the Flask web UI (default `5000`).

For each setting the application uses the following precedence:
1. Value from the configuration file
2. Environment variable override (`MANDEYE_LIVOX_LISTEN_IP`, `MANDEYE_REPO`, `SERVER_PORT`, `WEB_PORT`)
3. Compiled default embedded in the binaries

Edit the configuration file or set environment variables as needed before
starting the application.
