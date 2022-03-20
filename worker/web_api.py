#!/usr/bin/python

from flask import Flask,request
from web_utils import *
from worker import Worker
from sdfec import ldpc_code

app = Flask(__name__)
worker = None

@app.route("/")
def index():
    with open("index.html", "r") as f:
        data = f.read()

    data.replace('${CONFIG_HOST}', ber_config["ip_address"])
    data.replace('${CONFIG_PORT}', ber_config["port"])

@app.route("/api/codes", methods=['GET'])
def get_codes():
    """
    Returns a list of available and loaded codes, or rather their IDs.
    When creating a new simulation task, only values which are part of
    this list may be specified for the code id!
    Add new entries by executing a PUT against /code.
    """
    return jsonify(list(worker.codes.keys()))

@app.route("/api/code/<code_id>", methods=['GET'])
def get_code(code_id):
    """
    Returns information about a particular code, or 404 if no code with
    the provided id was found.
    """
    if code_id not in worker.codes:
        return Response(status=404)

    return jsonify(worker.codes[code_id])

@app.route("/api/code", methods=['PUT'])
def put_code():
    """
    Add a new code to the device. This endpoint will perform some validation, and
    attempt to append the code to the list of valid codes. On success, the code id
    will be returned as part of the response.

    Adding a code which is already present results in no action being taken on the
    server side, and the same response as with a sucessful operation, except for
    a response code of 200 instead of 201 for new codes.

    Additionally, code ids are derived from the hash of the json representation of
    the code, thus the ID will always stay the same for a particular code. MD5 was
    chosen for the hashing function, and while not adequate for cryptographic use,
    it will protect against accidental collisions.
    """
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
    """
    Returns internal status information of the worker. This can be used to gain some
    insight into what the decoders and BERTs are doing, but should not be considered
    a fixed part of the API.
    """
    return jsonify(worker.status)

@app.route("/api/results", methods=['GET'])
def get_results():
    """
    Returns the list of available results. For every submitted task a task_id is
    generated based on the task specification, where two identical tasks will have the
    same task id. These task ids are then also used to identify the results as they
    come in.
    """
    return jsonify(list(worker.results.keys()))

@app.route("/api/task", methods=["PUT"])
def put_task():
    """
    This endpoint can be used to create new simulation tasks. A simulation task
    containts information about which code should be simulated, at which SNRs it should
    be simulated and how the decoders should be configured. The parameters are sent
    as a json object:

    * code_id: str, Code ID as returned by PUT /code
    * snrs: double[], List of SNRs which should be tested.
    * snr_scales: double[], List of LLR scaling factors.
    * term_time: int, Maximum time in seconds which should be spent for each SNR value.
    * term_errors: int, Maximum amount of bit errors which should be collected for each SNR value.
    * max_iterations: int, 0 < x < 64, Maximum count of decoder iterations
    * collect_last_failed: int, Maximum amount of failed block IDs which should be collected for each SNR value.

    Definition of SNR:
    
    Note: Here, the SNR is defined as the E_s / sigma^2 for a BPSK with E_S = 1 and a
    real gaussian noise process. I.e. N_0 = sigma^2, NOT N_0 = 2*sigma^2.
    """
    body = request.json

    if body is None:
        return jsonify({"message": "Bad data or missing content type!"}, code=400)

    task = SimulationTask(**body)

    if task.task_id in worker.tasks:
        return jsonify({"id": task.task_id, "message": "Task already present!"}, code=200)

    if task.code_id not in worker.codes:
        return jsonify({"message": "Code not available!"}, code=404)

    worker.submit_task(task)
    return jsonify({"id": task.task_id}, 201)

@app.route("/api/task/<task_id>", methods=["GET"])
def get_task(task_id):
    """
    This endpoint returns information about the current state of a task.
    """
    if task_id not in worker.tasks:
        return Response(status=404)

    return jsonify(worker.tasks[task_id])

@app.route("/api/tasks", methods=["GET"])
def get_tasks():
    """
    Return a list of all registered tasks. These are either in process or queued up,
    once a task has completed, it will be deleted from the task list and be available
    for retrieval from the results.
    """
    return jsonify(list(worker.tasks.keys()))

@app.route("/api/task/<task_id>", methods=["DELETE"])
def delete_task(task_id):
    """
    If you want to cancel a task that is either still in the queue, or already running,
    you can do so using this method.
    """
    if task_id not in worker.tasks:
        return ResponseCode(status=404)

    del worker.tasks[task_id]

    worker.cancelled_tasks.append(task_id)
    return Response(status=200)

@app.route("/api/result/<task_id>", methods=["DELETE"])
def delete_result(task_id):
    """
    Should you want to save some memory or *rerun* a simulation, you need to delete the
    old result from the worker to prevent the caching mechanism from taking over. This
    can be achieved using this endpoint.
    """
    if task_id in worker.results:
        del worker.results[task_id]
        return Response(status=200)
    return Response(status=404)

@app.route("/api/result/<task_id>", methods=["GET"])
def get_result(task_id):
    """
    Return information about the results of a completed task. The ID used to index the
    tasks here is the same as it is for the tasks, except that only completed tasks
    have result informtion available to be retrieved.

    Result data is returned as a json object with the following fields:

    * task_id: str, Task id corresponding to the results object
    * success: bool, whether everything went as planned
    * snrs: double[], List of SNR values which were tested
    * bers: double[], The corresponding bit error rate values
    * speeds: double[], How many codeword LLRs were simulated per second
    * finished_blocks: int[], How many code blocks were simulated for the individual SNRs
    * bit_errors: int[], The total amount of bit_errors that were encountered for each SNR
    * avg_iterations: double[], How many iterations were required on average for each SNR
    * failed_blocks: int[], Total amount of blocks encountered for which the parity did not pass
    * last_failed: int[][][], This three-dimensional array is a large list of block IDs
                   of blocks that the decoder failed to decode. This data can be used
                   to reconstruct the input data in software and run it through a
                   bit-accurate model of the decoder, which allows for the full
                   reconstruction of the event. Dimensions: [SNRs][Decoder][i]
                   The Decoder dimension identifies which decoder in the hardware was
                   responsible for producing that particular list of mistakes. Because
                   the seeds of the random number generators differ for each decoder,
                   this information is required to correctly reconstruct the input.

    """
    if task_id in worker.results:
        return jsonify(worker.results[task_id])
    return Response(status=404)

def run(config, debug, dry_run=False):
    global worker
    global ber_config
    ber_config = config
    worker = Worker(config, debug, dry_run)
    app.run(host=config["ip_address"], port=config["port"], debug=debug)
