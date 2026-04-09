"""
BLE Locator — Smartphone Companion App

Flask web server that serves a Web Bluetooth PWA.
The smartphone connects to the micro:bit via BLE GATT,
receives position notifications, and displays them on a floor map.

Usage:
    pip install -r requirements.txt
    python app.py
    Open http://localhost:8080 on Android Chrome
"""
from flask import Flask, render_template

app = Flask(__name__)


@app.route("/")
def index():
    return render_template("index.html")


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8080, debug=True)
