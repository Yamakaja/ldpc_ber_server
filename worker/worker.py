#!/usr/bin/python

import hashlib
import numpy as np
from queue import Queue
from threading import Thread
import time

import sdfec
from ldpc_ber_tester import LDPCBERTester

class SimulationTask:

    def __init__(self, code_id, snrs, snr_scales=[], term_time=60, term_errors=1000, max_iterations=8):
        self.code_id = code_id
        self.snrs = snrs
        self.snr_scales = snr_scales

        if len(self.snr_scales) == 0:
            self.snr_scales = [1.0] * len(self.snrs)

        self.term_time = term_time
        self.term_errors = term_errors
        self.max_iterations = max_iterations

        self.task_id = hashlib.md5(f"{self.code_id}, {self.snrs}, {self.snr_scales}, "
            "{self.term_time}, {self.term_errors}, {self.max_iterations}".encode("UTF-8")).hexdigest()

        self.eta = self.term_time * len(self.snrs)
        self.progress = 0

        self.state = "INITIALIZED"

    def __repr__(self):
        return str(self.__dict__)

class TaskResult:

    def __init__(self, task_id, success: bool, message: str = "", snrs=[], bers=[], speeds=[], finished_blocks=[], bit_errors=[]):
        self.task_id = task_id
        self.success = success

        self.snrs = list(snrs)
        self.bers = bers

        self.speeds = speeds
        self.finished_blocks = finished_blocks
        self.bit_errors = bit_errors

    def __repr__(self):
        return str(self.__dict__)

class Worker:

    def __init__(self, config, debug, dry_run=False):
        self.config = config
        self.debug = debug

        if not dry_run:
            self.sdfecs = [sdfec.sdfec(i, True) for i in config["sdfec"]]
            self.ber_testers = [LDPCBERTester(p) for p in config["ber_tester"]]
        else:
            self.sdfecs = []
            self.ber_testers = []

        self.running = True

        self.task_queue = Queue()
        self.tasks = {}
        self.results = {}
        self.current_task = None

        self.loaded_code = None
        self.codes = {}

        self.cancelled_tasks = []

        Thread(target=self.simulation_worker, name="SDFEC Simulation Worker").start()

    def join(self):
        self.running = False
        return self.task_queue.join()

    def simulation_worker(self):
        def dbg_msg(v):
            if self.debug:
                print("SimulationWorker:", v)

        while self.running or not self.task_queue.empty():
            try:
                task = self.task_queue.get(timeout=0.1)
            except:
                self.cancelled_tasks.clear()
                continue

            dbg_msg("Got new task!")

            if task.task_id in self.cancelled_tasks:
                task.state = "CANCELLED"
                self.cancelled_tasks.remove(task.task_id)
                continue

            if task.task_id in self.results:
                self.task_queue.task_done()
                task.state = "DONE"
                continue

            if task.code_id not in self.codes:
                print("[ERROR] Task with invalid code_id in queue!")
                self.task_queue.task_done()
                task.state = "INVALID"
                continue

            code = self.codes[task.code_id]

            if self.loaded_code != task.code_id:
                dbg_msg("Loading new code ...")
                for fec in self.sdfecs:
                    dbg_msg(f"Pre-Task SDFEC state: {fec.state}")
                    fec.set_ldpc_code(code)

            for ber in self.ber_testers:
                dbg_msg(f"Pre-Task BER enable: {ber.enable}")
                ber.max_iterations = task.max_iterations
                ber.n = code.n
                ber.k = code.k

            self.loaded_code = task.code_id

            result_bers = []
            result_speeds = []
            result_finished_blocks = []
            result_bit_errors = []

            self.current_task = task
            task.progress = 0
            task.eta = task.term_time * len(task.snrs)
            i = 0

            task.state = "RUNNING"

            for snr,snr_scale in zip(task.snrs, task.snr_scales):
                for fec in self.sdfecs:
                    fec.start()

                start_time = time.time()
                for ber in self.ber_testers:
                    ber.snr_scale = snr_scale
                    ber.snr = snr
                    ber.enable = True

                while True:
                    if task.task_id in self.cancelled_tasks:
                        break

                    total_errors = sum([ber.bit_errors for ber in self.ber_testers])
                    remaining_time = start_time + task.term_time - time.time()
                    passed_time = time.time() - start_time

                    if total_errors > task.term_errors or remaining_time <= 0:
                        break

                    dbg_msg(f"Running - Status:")
                    dbg_msg(f"Current BER: {self.get_ber()}")

                    # Update ETA & Progress

                    # Inter SNR progress
                    iprogress = max(total_errors / task.term_errors, passed_time / task.term_time)
                    task.progress = (i+iprogress) / len(task.snrs)
                    eta = (len(task.snrs)-1-i) * task.term_time
                    if total_errors == 0 or passed_time == 0:
                        task.eta = eta + remaining_time
                    else:
                        task.eta = eta + min(remaining_time, (task.term_errors - total_errors) / (total_errors / passed_time))
                    
                    if self.debug:
                        for ber in self.ber_testers:
                            dbg_msg(f" - FEC {ber.core_id}")
                            dbg_msg(f"   finished_blocks: {ber.finished_blocks}")
                            dbg_msg(f"   bit_errors:      {ber.bit_errors}")
                            dbg_msg(f"   in_flight:       {ber.in_flight}")
                            dbg_msg(f"   last_status:     {hex(ber.last_status)}")

                    time.sleep(1)

                for ber in self.ber_testers:
                    ber.enable = False

                while any([fec.active for fec in self.sdfecs]):
                    dbg_msg("Waiting for SDFECs to finish ...")
                    time.sleep(1)

                dbg_msg("Done!")

                for fec in self.sdfecs:
                    fec.stop()

                stop_time = time.time()

                current_ber = self.get_ber()
                print(f"Results are in: SNR = {snr}, BER = {current_ber}")

                result_bers.append(current_ber)

                finished_blocks = sum([ber.finished_blocks for ber in self.ber_testers])
                bit_errors = sum([ber.bit_errors for ber in self.ber_testers])
                result_speeds.append(code.n * finished_blocks / (stop_time - start_time))
                result_finished_blocks.append(finished_blocks)
                result_bit_errors.append(bit_errors)

                for ber in self.ber_testers:
                    ber.reset()

                i += 1

                if task.task_id in self.cancelled_tasks:
                    break

            if task.task_id in self.cancelled_tasks:
                self.cancelled_tasks.remove(task.task_id)
                self.task_queue.task_done()
                self.current_task = None

                task.state = "CANCELLED"
                continue

            assert len(result_bers) == len(task.snrs)

            self.current_task = None

            result = TaskResult(
                    task_id=task.task_id,
                    success=True,
                    message="Success!",
                    snrs=task.snrs,
                    bers=result_bers,
                    speeds=result_speeds,
                    finished_blocks=result_finished_blocks,
                    bit_errors=result_bit_errors)

            self.results[task.task_id] = result

            task.state = "DONE"

            print("Completed task:", task.task_id)
            self.task_queue.task_done()

    def submit_task(self, task):
        task.state = "WAITING"
        self.tasks[task.task_id] = task
        self.task_queue.put(task)

    def get_ber(self):
        current_ber = np.array([np.array([ber.bit_errors,ber.finished_blocks]) for ber in self.ber_testers], dtype=np.int64)
        if len(self.ber_testers) == 0 or np.sum(current_ber[:,1]) == 0 or current_ber.size == 0:
            return 0

        current_ber = np.sum(current_ber[:,0]) / (self.codes[self.loaded_code].k * np.sum(current_ber[:,1]))
        return current_ber

    def add_code(self, codedef):
        if not isinstance(codedef, dict) or "dec_OK" not in codedef or not codedef["dec_OK"]:
            raise RuntimeError("Attempted to register code which cannot be decoded!")

        def _str_to_array(v):
            return [int(x) for x in v.strip().split(" ") if len(x) > 0]

        code = sdfec.ldpc_code()

        code.dec_OK = codedef["dec_OK"]
        code.enc_OK = codedef["enc_OK"]

        code.n = codedef["n"]
        code.k = codedef["k"]
        code.p = codedef["p"]
        code.nlayers = codedef["nlayers"]
        code.nqc = codedef["nqc"]
        code.nmqc = codedef["nmqc"]
        code.nm = codedef["nm"]
        code.norm_type = codedef["norm_type"]
        code.no_packing = codedef["no_packing"]
        code.special_qc = codedef["special_qc"]
        code.no_final_parity = codedef["no_final_parity"]
        code.max_schedule = codedef["max_schedule"]

        code.sc_table = _str_to_array(codedef["sc_table"])
        code.la_table = _str_to_array(codedef["la_table"])
        code.qc_table = _str_to_array(codedef["qc_table"])

        self.codes[code.hash] = code

        return code.hash

    @property
    def status(self):
        ret = {
                "name": self.config["name"],
                "running": self.running,
                "current_task": self.current_task.task_id
                }

        ret["loaded_code"] = self.loaded_code

        ret["sdfecs"] = [{
                "id": fec.id,
                "active": fec.active,
                "state": fec.state,
                "bypass": fec.bypass
            } for fec in self.sdfecs]

        ret["ber_testers"] = [{
                "id": ber.core_id,
                "version": ber.version,
                "scratch": ber.scratch,
                "enable": ber.enable,
                "snr": ber.snr,
                "snr_scale": ber.snr_scale,
                "n": ber.n,
                "k": ber.k,
                "finished_blocks": ber.finished_blocks,
                "bit_errors": ber.bit_errors,
                "in_flight": ber.in_flight,
                "last_status": ber.last_status
            } for ber in self.ber_testers]

        return ret

