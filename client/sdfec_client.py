#!/usr/bin/python3

import yaml
import requests
import time
import sdfec_cmodel
import numpy as np
import tempfile

# Keep references to tempfiles so they are deleted on application shutdown
_tempfiles = []

try:
    in_jupyter = get_ipython().__class__.__name__ == "ZMQInteractiveShell"
except:
    in_jupyter = False

if in_jupyter:
    from ipywidgets import FloatProgress
    from IPython.display import display

def parse_QCldpc(file, row_normalization=True, normalization=0.75, no_packing=False, just_encode=False):
    with open(file, "r") as f:
        lines = f.read().split("\n")

    rows = int(lines[0])
    cols = int(lines[1])
    psize = int(lines[2])

    HQC = np.array([[int(v) for v in row.split(" ") if len(v.strip()) > 0] for row in lines[3:3+rows] if len(row.strip()) > 0])
    assert HQC.shape == (rows, cols)
    assert lines[3+rows] == "EOF"

    data = {}

    assert cols > rows
    assert psize > 0

    data["n"] = cols * psize
    data["k"] = (cols-rows) * psize

    data["p"] = psize
    data["normalization"] = "row" if row_normalization else "none"

    data["scale"] = round(normalization * 12)

    data["no_packing"] = no_packing
    data["encode"] = just_encode

    circulants = []
    R,C = np.meshgrid(np.arange(rows), np.arange(cols), indexing="ij")

    R,C = R[HQC >= 0], C[HQC >= 0]

    for row,col in zip(R.reshape(R.size), C.reshape(C.size)):
        q = HQC[row, col]
        circulants.append({"row": int(row), "col": int(col), "shift": int(q)})

    data["sm_array"] = circulants

    outfile = tempfile.NamedTemporaryFile(suffix=".yml")
    outfile.write(yaml.safe_dump(data).encode("UTF-8"))
    outfile.flush()
    _tempfiles.append(outfile)

    return outfile.name


class SDFECTaskResult:
    def __init__(self, task, data, max_iterations, snr_scales, cmodel):
        self.task = task
        self.data = data

        self.task_id = data["task_id"]
        self.success = data["success"]
        self.snrs = data["snrs"]
        self.bers = data["bers"]
        self.speeds = data["speeds"]
        self.finished_blocks = data["finished_blocks"]
        self.bit_errors = data["bit_errors"]
        self.avg_iterations = data["avg_iterations"]
        self.failed_blocks = data["failed_blocks"]

        self.fers = np.array(self.failed_blocks, dtype=float) / np.array(self.finished_blocks, dtype=float)

        if cmodel != None and "last_failed" in data and isinstance(data["last_failed"], list):
            failed_code_words = []
            failed_info_words = []

            ber_testers = [sdfec_cmodel.ldpc_ber_tester(i, cmodel) for i in range(6)]

            last_failed = data["last_failed"]

            N = sum([sum([len(v) for v in u]) for u in last_failed])
            passed = 0
            print(f"Processing {N} blocks in local cmodel:")

            if in_jupyter:
                fp = FloatProgress(min=0, max=1, bar_style="info", description=f"{N}")
                display(fp)

            done = 0

            start_time = time.time()
            for decoders,snr,snr_scale in zip(last_failed, self.snrs, snr_scales):
                code_words = None
                info_words = None

                for i,decoder,ber in zip(range(6), decoders, ber_testers):
                    ber.snr = snr
                    ber.snr_scale = snr_scale


                    for block_id in decoder:
                        din, status, _, dout_hd = ber.simulate_block(block_id, max_iterations=max_iterations)
                        if status.passed:
                           print(f"Warning: Block passed! decoder: {i}, block_id: {block_id}, snr: {snr}, snr_scale: {snr_scale}, status: {status}")
                           passed += 1
                           done += 1
                           continue

                        if code_words is None:
                            code_words = np.array(din)+snr_scale
                            info_words = np.array(dout_hd).astype(int)
                        else:
                            code_words += np.array(din, copy=False)+snr_scale
                            info_words += np.array(dout_hd, copy=False)

                        done += 1

                        if in_jupyter and (done & 0xF) == 0:
                            duration = time.time() - start_time
                            eta = int(round((N-done)/ done * duration))
                            seconds = eta % 60
                            eta //= 60
                            minutes = eta % 60
                            eta //= 60
                            hours = eta
                            fp.value = done / N
                            fp.description = f"{hours:02}:{minutes:02}:{seconds:02}"

                failed_code_words.append(code_words)
                failed_info_words.append(info_words)

            print(f"Passed: {passed} / {N}")
            if in_jupyter:
                fp.value = 1
                fp.description = "Done"

            self.failed_info_words = failed_info_words
            self.failed_code_words = failed_code_words

class SDFECTask:
    def __init__(self, parent, task_id, max_iterations, snr_scales, cmodel=None):
        self._parent = parent
        self.task_id = task_id
        self.cmodel = cmodel
        self.max_iterations = max_iterations
        self.snr_scales = snr_scales

        self.update()

    def update(self):
        self.data = self._parent._get(f"task/{self.task_id}")

    @property
    def progress(self):
        return self.data["progress"]

    @property
    def eta(self):
        return self.data["eta"]

    @property
    def state(self):
        return self.data["state"]

    @property
    def eta_str(self):
        v = int(self.eta)

        seconds = v % 60
        v //= 60

        minutes = v % 60
        v //= 60

        hours = v

        return f"{hours:02}:{minutes:02}:{seconds:02}"

    @property
    def result(self):
        return SDFECTaskResult(self, self._parent._get(f"result/{self.task_id}"), self.max_iterations, self.snr_scales, self.cmodel)

    def wait(self):
        while self.state in ["WAITING", "RUNNING"]:
            time.sleep(1)
            self.update()

        if self.state == "DONE":
            return self.result
        return None

    def wait_progress(self):
        if not in_jupyter:
            return self.wait()

        fp = FloatProgress(min=0, max=1, bar_style="info", description=f"{self.eta_str}")

        if self.state == "WAITING":
            print("Queued, waiting ...")

        while self.state == "WAITING":
            time.sleep(1)
            self.update()

        display(fp)

        while self.state == "RUNNING":
            time.sleep(1)
            self.update()
            fp.value = self.progress
            fp.description = f"{self.eta_str}"

        fp.value = 1
        fp.description = "Done"

        if self.state == "DONE":
            return self.result

        raise RuntimeError(f"ABORT, task not done, but not running? data: {self.data}")

class SDFECClient:
    def __init__(self, base_url):
        self.base_url = base_url
        self._tasks = {}
        self.code_files = {}

    def _get(self, endpoint):
        req = requests.get(f"{self.base_url}/{endpoint}")
        if req.ok:
            return req.json()

        if req.json() is not None and "message" in req.json():
            msg = req.json()["message"]
            raise RuntimeError(f"Failed to GET endpoint {endpoint}: {msg}")
        raise RuntimeError(f"Failed to GET endpoint: {endpoint}")

    def _put(self, endpoint, data):
        req = requests.put(f"{self.base_url}/{endpoint}", json=data)
        if req.ok:
            return req.json()

        print(req.content)

        if req.json() is not None and "message" in req.json():
            msg = req.json()["message"]
            raise RuntimeError(f"Failed to PUT endpoint {endpoint}: {msg}")
        raise RuntimeError(f"Failed to PUT endpoint: {endpoint}")

    def _delete(self, endpoint):
        req = requests.delete(f"{self.base_url}/{endpoint}");
        if req.ok:
            return req.json()

        if req.json() is not None and "message" in req.json():
            msg = req.json()["message"]
            raise RuntimeError(f"Failed to DELETE endpoint {endpoint}: {msg}")
        raise RuntimeError(f"Failed to DELETE endpoint: {endpoint}")

    @property
    def tasks(self):
        return self._get("codes")

    def add_code(self, code_spec):
        """
        Parse code specifications. Accepts Xilinx yml format (Before going through vivado) and QCldpc files.
        """

        if code_spec.endswith(".QCldpc"):
            code_spec = parse_QCldpc(code_spec)

        code = sdfec_cmodel.gen_ldpc_params(code_spec)
        code_id = self._put(f"code", code.dict)["id"]
        self.code_files[code_id] = code_spec
        return code_id, code

    def simulate(self, code_id, snrs, snr_scales=[], term_time=60, term_errors=1000, max_iterations=8, collect_last_failed=0):
        if len(snr_scales) == 0:
            snr_scales = [1.0] * len(snrs)

        req = {
                "code_id": code_id,
                "snrs": list(snrs),
                "snr_scales": list(snr_scales),
                "term_time": term_time,
                "term_errors": term_errors,
                "max_iterations": max_iterations,
                "collect_last_failed": collect_last_failed
                }


        task_id = self._put("task", req)["id"]
        if collect_last_failed > 0:
            cmodel = sdfec_cmodel.sdfec_core(f"[SDFEC: task={task_id}]", self.code_files[code_id]);
        else:
            cmodel = None

        return SDFECTask(self, task_id, max_iterations, snr_scales, cmodel)

    @property
    def status(self):
        return self._get("status")

    @property
    def results(self):
        return self._get("results")

    @property
    def tasks(self):
        return self._get("tasks")

    @property
    def codes(self):
        return self._get("codes")

    def get_result(self, task_id):
        return self._get(f"result/{task_id}")

    def get_task(self, task_id):
        if task_id in self._tasks:
            task = self._tasks[task_id]
            task.update()
            return task

        return SDFECTask(self, task_id)

    def get_code(self, code_id):
        return self._get(f"code/{code_id}")

    def delete_result(self, task_id):
        return self._delete(f"result/{task_id}")

    def delete_task(self, task_id):
        return self._delete(f"task/{task_id}")

    def delete_code(self, code_id):
        return self._delete(f"code/{code_id}")

