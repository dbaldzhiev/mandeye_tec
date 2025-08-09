import ctypes
import os
import atexit
import json
import threading
import time
from flask import Flask, render_template, jsonify

LIB_PATH = os.path.join(os.path.dirname(__file__), '..', 'build', 'libmandeye_core.so')
lib = ctypes.CDLL(LIB_PATH)

lib.Init()
atexit.register(lib.Shutdown)

lib.StartScan.restype = ctypes.c_bool
lib.StopScan.restype = ctypes.c_bool
lib.TriggerStopScan.restype = ctypes.c_bool
lib.produceReport.argtypes = [ctypes.c_bool]
lib.produceReport.restype = ctypes.c_char_p

# cache for status information updated in the background
status_cache = {}


def poll_status():
    """Poll the C++ layer for status information."""
    global status_cache
    while True:
        raw = lib.produceReport(ctypes.c_bool(True)).decode("utf-8")
        try:
            status_cache = json.loads(raw)
        except json.JSONDecodeError:
            status_cache = {}
        time.sleep(1)


threading.Thread(target=poll_status, daemon=True).start()

app = Flask(__name__)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/status')
def status():
    return jsonify(status_cache)

@app.route('/status_full')
def status_full():
    raw = lib.produceReport(ctypes.c_bool(True)).decode("utf-8")
    return jsonify(json.loads(raw))

@app.route('/start_scan')
def start_scan():
    lib.StartScan()
    return '', 204

@app.route('/stop_scan')
def stop_scan():
    lib.StopScan()
    return '', 204

@app.route('/stopscan')
def stopscan():
    lib.TriggerStopScan()
    return '', 204

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
