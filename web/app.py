import ctypes
import os
import atexit
from flask import Flask, render_template

LIB_PATH = os.path.join(os.path.dirname(__file__), '..', 'build', 'libmandeye_core.so')
lib = ctypes.CDLL(LIB_PATH)

lib.Init()
atexit.register(lib.Shutdown)

lib.StartScan.restype = ctypes.c_bool
lib.StopScan.restype = ctypes.c_bool
lib.TriggerStopScan.restype = ctypes.c_bool
lib.produceReport.argtypes = [ctypes.c_bool]
lib.produceReport.restype = ctypes.c_char_p

app = Flask(__name__)

@app.route('/')
def index():
    status = lib.produceReport(ctypes.c_bool(False)).decode('utf-8')
    return render_template('index.html', status=status)

@app.route('/status')
def status():
    return lib.produceReport(ctypes.c_bool(False)).decode('utf-8')

@app.route('/status_full')
def status_full():
    return lib.produceReport(ctypes.c_bool(True)).decode('utf-8')

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
