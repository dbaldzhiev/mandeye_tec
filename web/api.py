import ctypes
import json
import zmq
from flask import Blueprint, Response, jsonify, current_app
from backend import lib, status_cache

api_bp = Blueprint('api', __name__, url_prefix='/api')


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
