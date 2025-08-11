import ctypes
import json
import os
import time
import zmq
from collections import deque
from flask import Blueprint, Response, jsonify, current_app, request
from backend import lib, status_cache, config

api_bp = Blueprint('api', __name__, url_prefix='/api')

LOG_FILE = os.path.join(os.path.dirname(__file__), '..', 'logs', 'mandeye.log')


@api_bp.route('/status')
def status():
    return jsonify(status_cache)


@api_bp.route('/status_full')
def status_full():
    raw = lib.produceReport(ctypes.c_bool(True)).decode("utf-8")
    return jsonify(json.loads(raw))


@api_bp.route('/start_scan', methods=['POST'])
def start_scan():
    if lib.StartScan():
        return '', 204
    return jsonify({'error': 'unable to start scan'}), 400


@api_bp.route('/stop_scan', methods=['POST'])
def stop_scan():
    if lib.StopScan():
        return '', 204
    return jsonify({'error': 'unable to stop scan'}), 400


@api_bp.route('/stopscan', methods=['POST'])
def stopscan():
    if lib.TriggerStopScan():
        return '', 204
    return jsonify({'error': 'unable to trigger stop scan'}), 400


@api_bp.route('/config', methods=['POST'])
def update_config():
    data = request.get_json(force=True)
    size = int(data.get('chunk_size_mb', config.get('chunk_size_mb', 0)))
    duration = int(data.get('chunk_duration_s', config.get('chunk_duration_s', 0)))
    lib.SetChunkOptions(duration, size)
    config['chunk_size_mb'] = size
    config['chunk_duration_s'] = duration
    return '', 204


def zmq_event_stream(port):
    context = zmq.Context()
    socket = context.socket(zmq.SUB)
    socket.connect(f"tcp://localhost:{port}")
    socket.setsockopt_string(zmq.SUBSCRIBE, '')
    try:
        while True:
            message = socket.recv_string()
            yield f"data: {message}\n\n"
    finally:
        socket.close()
        context.term()


@api_bp.route('/stream')
def stream():
    port = current_app.config.get('server_port', 8003)
    return Response(zmq_event_stream(port), mimetype='text/event-stream')


def log_event_stream():
    def generate():
        os.makedirs(os.path.dirname(LOG_FILE), exist_ok=True)
        try:
            with open(LOG_FILE, 'r') as f:
                buf = deque(f, maxlen=200)
                for line in buf:
                    yield f"data: {line.rstrip()}\n\n"
                while True:
                    line = f.readline()
                    if line:
                        yield f"data: {line.rstrip()}\n\n"
                    else:
                        time.sleep(1)
        except FileNotFoundError:
            pass
    return Response(generate(), mimetype='text/event-stream')


@api_bp.route('/logs')
def logs():
    return log_event_stream()


@api_bp.route('/recordings')
def recordings():
    from usb_utils import find_usb_mount

    repo = find_usb_mount()
    recs = []
    if repo:
        try:
            for entry in os.scandir(repo):
                if entry.is_dir():
                    recs.append({'folder': entry.name})
        except OSError:
            pass
    return jsonify({'recordings': recs})
