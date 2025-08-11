import ctypes
import os
import atexit
import json
import threading
import time
import subprocess
import shutil
import re
import socket
import errno

CONFIG_PATH = os.path.join(os.path.dirname(__file__), '..', 'config', 'mandeye_config.json')


def load_config():
    try:
        with open(CONFIG_PATH) as f:
            return json.load(f)
    except OSError:
        return {}


config = load_config()

# Try to load the C++ recording backend.  In development environments the
# shared library may not be present; we still want the web UI to start so we
# handle this gracefully and provide no-op fallbacks.
LIB_PATH = os.path.join(os.path.dirname(__file__), '..', 'build', 'libmandeye_core.so')
try:
    lib = ctypes.CDLL(LIB_PATH)
    lib.Init()
    atexit.register(lib.Shutdown)
    lib.StartScan.restype = ctypes.c_bool
    lib.StopScan.restype = ctypes.c_bool
    lib.TriggerStopScan.restype = ctypes.c_bool
    lib.produceReport.argtypes = [ctypes.c_bool]
    lib.produceReport.restype = ctypes.c_char_p
    lib.SetChunkOptions.argtypes = [ctypes.c_int, ctypes.c_int]
except OSError:
    class _Stub:
        def __getattr__(self, name):
            if name == 'produceReport':
                return lambda *args, **kwargs: b'{}'
            return lambda *args, **kwargs: False

    lib = _Stub()

status_cache = {}


def poll_status():
    """Poll the C++ backend for status and update ``status_cache``.

    The Livox/LiDAR section of the report can become stale if the device is
    unplugged or stops acknowledging messages.  To provide an accurate
    connection state to the frontend, we monitor the timestamp reported by the
    device and clear the detailed information if it hasn't updated recently.
    """
    global status_cache
    last_probe = 0
    probe_reachable = False
    while True:
        raw = lib.produceReport(ctypes.c_bool(True)).decode("utf-8")
        try:
            data = json.loads(raw)
        except json.JSONDecodeError:
            data = {}

        livox = data.get("livox", {})
        now = time.time()
        ts = livox.get("LivoxLidarInfo", {}).get("timestamp_s")
        init_success = livox.get("init_success")

        connected = bool(init_success and ts and now - ts < 3)

        if not connected:
            # Either the LiDAR was never initialised or we haven't received
            # an acknowledgement in a while – clear any stale details so the
            # UI can show a disconnected state.
            data["livox"] = {"init_success": False}

        # Augment report with additional system information used by the UI.
        repo = config.get('repository_path', '/media/usb/')
        storage_present = os.path.ismount(repo) or os.path.exists(repo)
        free_space = None
        if storage_present:
            try:
                free_space = shutil.disk_usage(repo).free
            except OSError:
                storage_present = False
        data['storage_present'] = storage_present
        data['free_space'] = free_space

        def get_ip(iface):
            try:
                out = subprocess.check_output(['ip', '-4', 'addr', 'show', iface], text=True)
                m = re.search(r'inet (\d+\.\d+\.\d+\.\d+)', out)
                return m.group(1) if m else None
            except Exception:
                return None

        eth0_ip = get_ip('eth0')
        data['eth0_ip'] = eth0_ip
        lidar_ip = config.get('livox_interface_ip')
        data['lidar_ip'] = lidar_ip

        if connected:
            probe_reachable = True
        elif lidar_ip:
            if now - last_probe > 10:
                last_probe = now
                try:
                    with socket.create_connection((lidar_ip, 80), timeout=0.2):
                        probe_reachable = True
                except OSError as e:
                    probe_reachable = e.errno == errno.ECONNREFUSED
        else:
            probe_reachable = False

        lidar_detected = connected or probe_reachable
        data['lidar_detected'] = lidar_detected
        if lidar_detected:
            data['lidar_state'] = 'connected' if connected else 'unreachable'
        else:
            data['lidar_state'] = 'offline'
        data['chunk_size_mb'] = config.get('chunk_size_mb')
        data['chunk_duration_s'] = config.get('chunk_duration_s')

        status_cache = data
        time.sleep(1)


threading.Thread(target=poll_status, daemon=True).start()
