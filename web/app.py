import os
from flask import Flask
from backend import config
from api import api_bp
from ui import ui_bp

def create_app() -> Flask:
    app = Flask(__name__)
    app.config.update(config)
    app.register_blueprint(api_bp)
    app.register_blueprint(ui_bp)
    return app

app = create_app()

if __name__ == "__main__":
    port = config.get("web_port")
    if port is None:
        port = int(os.environ.get("WEB_PORT", 5000))
    app.run(host="0.0.0.0", port=port)
