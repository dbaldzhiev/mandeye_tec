import os
import atexit
import json
import threading
import time
import ctypes

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
    global status_cache
    while True:
        raw = lib.produceReport(ctypes.c_bool(True)).decode("utf-8")
        try:
            status_cache = json.loads(raw)
        except json.JSONDecodeError:
            status_cache = {}
        time.sleep(1)


threading.Thread(target=poll_status, daemon=True).start()
