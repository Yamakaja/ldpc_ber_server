#!/usr/bin/python3

import yaml
import requests
import time

try:
    in_jupyter = get_ipython().__class__.__name__ == "ZMQInteractiveShell"
except:
    in_jupyter = False

if in_jupyter:
    from ipywidgets import FloatProgress
    from IPython.display import display

def parse_yaml(x):
    with open(x, "r") as f:
        data = yaml.safe_load(f)

    def parse_array(v):
        if isinstance(v, int):
            return [v]

        if isinstance(v, str):
            return [int(i) for i in v.strip().split(" ") if len(i) > 0]

        if isinstance(v, list) and isinstance(v[0], int):
            return v

        raise RuntimeError("Failed to process table:", v)

    data["sc_table"] = parse_array(data["sc_table"])
    data["la_table"] = parse_array(data["la_table"])
    data["qc_table"] = parse_array(data["qc_table"])
    
    return data

class SDFECTask:
    def __init__(self, parent, task_id):
        self._parent = parent
        self.task_id = task_id

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
    def result(self):
        return self._parent._get(f"result/{self.task_id}")

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

        fp = FloatProgress(min=0, max=1, bar_style="info", description="Running: ")

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

        fp.value = 1

        if self.state == "DONE":
            return self.result

        return None

class SDFECClient:
    def __init__(self, base_url):
        self.base_url = base_url
        self._tasks = {}

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

    def add_code(self, code):
        return self._put(f"code", code)["id"]

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

        return SDFECTask(self, self._put("task", req)["id"])

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

