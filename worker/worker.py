#!/usr/bin/python

from threading import Thread
from queue import Queue
import time

import numpy as np
import sdfec
from ldpc_ber_tester import LDPCBERTester

class SimulationTask:

    def __init__(self, task_id, code_id, snrs, snr_scales=[], term_time=60, term_errors=1000, max_iterations=8):
        self.task_id = task_id
        self.code_id = code_id
        self.snrs = np.array(snrs)
        self.snr_scales = np.array(snr_scales)

        if len(self.snr_scales) == 0:
            self.snr_scales = np.ones(self.snrs.size, dtype=float)

        self.term_time = term_time
        self.term_errors = term_errors
        self.max_iterations = max_iterations

    def __repr__(self):
        return str(self.__dict__)

class TaskResult:

    def __init__(self, task_id, success: bool, message: str = "", snrs=[], bers=[], speeds=[]):
        self.task_id = task_id
        self.success = success

        self.snrs = snrs
        self.bers = bers

        self.speeds = speeds

    def __repr__(self):
        return str(self.__dict__)

class Worker:

    def __init__(self, config, debug):
        self.config = config
        self.debug = debug

        self.sdfecs = [sdfec.sdfec(i, True) for i in config["sdfec"]]
        self.ber_testers = [LDPCBERTester(p) for p in config["ber_tester"]]

        self.running = True

        self._tasks = Queue()
        self.results = {}

        self.loaded_code = None
        self.codes = {}

        Thread(target=self.simulation_worker, name="SDFEC Simulation Worker").start()

    def join(self):
        self.running = False
        return self._tasks.join()

    def simulation_worker(self):
        def dbg_msg(v):
            if self.debug:
                print("SimulationWorker:", v)

        while self.running or not self._tasks.empty():
            try:
                task = self._tasks.get(timeout=0.1)
            except:
                continue

            dbg_msg("Got new task!")

            if task.code_id not in self.codes:
                result = TaskResult(
                        task_id = task.task_id,
                        success = False,
                        message = "Requested code not available!"
                        )

                self.results[task.task_id] = result
                self._tasks.task_done()
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

            for snr,snr_scale in zip(task.snrs, task.snr_scales):
                for fec in self.sdfecs:
                    fec.start()

                for ber in self.ber_testers:
                    ber.snr_scale = snr_scale
                    ber.snr = snr
                    ber.enable = True

                finished_blocks = 0

                start_time = time.time()
                while sum([ber.bit_errors for ber in self.ber_testers]) < task.term_errors \
                      and start_time + task.term_time > time.time():
                    dbg_msg("Running - Status:")
                    dbg_msg(f"Current BER: {self.get_ber()}")

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
                result_speeds.append(code.n * finished_blocks / (stop_time - start_time))

                for ber in self.ber_testers:
                    ber.reset()

            assert len(result_bers) == len(task.snrs)


            result = TaskResult(
                    task_id=task.task_id,
                    success=True,
                    message="Success!",
                    snrs=task.snrs,
                    bers=result_bers,
                    speeds=result_speeds)

            self.results["task_id"] = result

            print("Completed task!")
            self._tasks.task_done()

    def submit_task(self, task):
        self._tasks.put(task)

    def get_ber(self):
        current_ber = np.array([np.array([ber.bit_errors,ber.finished_blocks]) for ber in self.ber_testers], dtype=np.int64)
        # print("current_ber: ", current_ber)
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
                "running": self.running
                }
        if self.loaded_code != None:
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

