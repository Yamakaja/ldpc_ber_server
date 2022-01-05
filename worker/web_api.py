#!/usr/bin/python

from flask import Flask
from web_utils import *
from worker import Worker

app = Flask(__name__)
worker = None

@app.route("/api/codes", methods=['GET'])
def get_codes():
    return jsonify(worker.codes)

@app.route("/api/status", methods=['GET'])
def get_status():
    return jsonify(worker.status)

def run(config, debug):
    global worker
    worker = Worker(config, debug)
    app.run(debug=debug)
