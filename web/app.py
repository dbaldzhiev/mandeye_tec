from flask import Flask
from api import api_bp
from ui import ui_bp


def create_app() -> Flask:
    app = Flask(__name__)
    app.register_blueprint(api_bp)
    app.register_blueprint(ui_bp)
    return app


app = create_app()

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
