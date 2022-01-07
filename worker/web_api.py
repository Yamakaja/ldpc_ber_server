#!/usr/bin/python

from flask import Flask,request
from web_utils import *
from worker import Worker
from sdfec import ldpc_code

app = Flask(__name__)
worker = None

@app.route("/api/codes", methods=['GET'])
def get_codes():
    return jsonify(list(worker.codes.keys()))

@app.route("/api/code/<code_id>", methods=['GET'])
def get_code(code_id):
    if code_id not in worker.codes:
        return Response(status=404)

    return jsonify(worker.codes[code_id])

@app.route("/api/code", methods=['PUT'])
def put_code():
    data = request.json
    if data is None:
        return jsonify({"message": "Bad data or missing content type!"}, code=400)

    code = ldpc_code()

    for k,v in data.items():
        code.__setattr__(k, v)

    if not code.valid:
        return jsonify({"message": "Invalid code definition!"}, status=400)

    if code.hash in worker.codes:
        return jsonify({"message": "Code already present!", "id": code.hash}, code=200)

    worker.codes[code.hash] = code
    return jsonify({"message": "Created!", "id": code.hash}, code=201)

@app.route("/api/status", methods=['GET'])
def get_status():
    return jsonify(worker.status)

@app.route("/api/results", methods=['GET'])
def get_results():
    return jsonify(list(worker.results.keys()))

@app.route("/api/task", methods=["PUT"])
def put_task():
    body = request.json

    if body is None:
        return jsonify({"message": "Bad data or missing content type!"}, code=400)

    task = SimulationTask(**body)

    if task.task_id in worker.results:
        result = worker.results[task.task_id]
        if result.success:
            return jsonify({"task_id": task.task_id, "message": "Results available!"}, code=200)

    if task.code_id not in worker.codes:
        return jsonify({"message": "Code not available!"}, code=404)

    worker.submit_task(task)
    return jsonify({"task_id": task.task_id}, 201)

@app.route("/api/task/<task_id>", methods=["DELETE"])
def delete_task(task_id):
    worker.cancelled_tasks.append(task_id)
    return Response(status=200)

@app.route("/api/result/<task_id>", methods=["DELETE"])
def delete_result(task_id):
    if task_id in worker.results:
        del worker.results[task_id]
        return Response(status=200)
    return Response(status=404)

@app.route("/api/result/<task_id>", methods=["GET"])
def get_result(task_id):
    if task_id in worker.results:
        return jsonify(worker.results[task_id])
    return Response(status=404)

def run(config, debug, dry_run=False):
    global worker
    print(dry_run)
    worker = Worker(config, debug, dry_run)
    app.run(host=config["ip_address"], port=config["port"], debug=debug)
