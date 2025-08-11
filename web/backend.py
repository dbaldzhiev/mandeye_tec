import ctypes
import os
import atexit
import json
import threading
import time

CONFIG_PATH = os.path.join(os.path.dirname(__file__), '..', 'config', 'mandeye_config.json')


def load_config():
    try:
        with open(CONFIG_PATH) as f:
            return json.load(f)
    except OSError:
        return {}


config = load_config()

LIB_PATH = os.path.join(os.path.dirname(__file__), '..', 'build', 'libmandeye_core.so')
lib = ctypes.CDLL(LIB_PATH)

lib.Init()
atexit.register(lib.Shutdown)

lib.StartScan.restype = ctypes.c_bool
lib.StopScan.restype = ctypes.c_bool
lib.TriggerStopScan.restype = ctypes.c_bool
lib.produceReport.argtypes = [ctypes.c_bool]
lib.produceReport.restype = ctypes.c_char_p

status_cache = {}


def poll_status():
    """Poll the C++ backend for status and update ``status_cache``.

    The Livox/LiDAR section of the report can become stale if the device is
    unplugged or stops acknowledging messages.  To provide an accurate
    connection state to the frontend, we monitor the timestamp reported by the
    device and clear the detailed information if it hasn't updated recently.
    """
    global status_cache
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

        if not (init_success and ts and now - ts < 3):
            # Either the LiDAR was never initialised or we haven't received
            # an acknowledgement in a while – clear any stale details so the
            # UI can show a disconnected state.
            data["livox"] = {"init_success": False}

        status_cache = data
        time.sleep(1)


threading.Thread(target=poll_status, daemon=True).start()
