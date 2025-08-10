import os
from flask import Blueprint, send_from_directory

ui_bp = Blueprint('ui', __name__)
_TEMPLATE_DIR = os.path.join(os.path.dirname(__file__), 'templates')


@ui_bp.route('/')
def index():
    return send_from_directory(_TEMPLATE_DIR, 'index.html')
