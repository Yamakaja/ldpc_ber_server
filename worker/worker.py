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
            self.snr_scales = np.ones(self.snrs, dtype=float)

        self.termination_time = term_time
        self.termination_errors = term_errors
        self.max_iterations = 8

class TaskResult:

    def __init__(self, task_id, success: bool, message: str = "", snrs=[], bers=[]):
        self.task_id = task_id
        self.success = success

        self.snrs = snrs
        self.bers = bers

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

    def simulation_worker(self):
        def dbg_msg(v):
            if self.debug:
                print("SimulationWorker:", v)

        while self.running:
            task = self._tasks.get()
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
                ber.n = code.n
                ber.k = code.k

            self.loaded_code = task.code_id

            result_bers = []

            for snr,snr_scale in zip(task.snrs, task.snr_scales):
                for fec in self.sdfecs:
                    fec.start()

                for ber in self.ber_testers:
                    ber.snr_scale = snr_scale
                    ber.snr = snr
                    ber.enable = True

                start_time = time.time()
                while sum([fec.bit_errors for fec in self.sdfecs]) < task.term_errors \
                      and start_time + task.term_time > time.time():
                    dbg_msg("Running - Status:")
                    dbg_msg(f"Current BER: {get_ber()}")

                    for ber in self.ber_testers:
                        dbg_msg(f" - FEC {fec.id}")
                        dbg_msg(f"   finished_blocks: {fec.finished_blocks}")
                        dbg_msg(f"   bit_errors:      {fec.bit_errors}")
                        dbg_msg(f"   in_flight:       {fec.in_flight}")
                        dbg_msg(f"   last_status:     {hex(fec.in_flight)}")
                    time.sleep(1)

                for ber in self.ber_testers:
                    ber.enable = False

                while any([fec.active for fec in self.sdfecs]):
                    dbg_msg("Waiting for SDFECs to finish ...")
                    time.sleep(0.01)

                dbg_msg("Done!")

                for fec in self.sdfecs:
                    fec.stop()

                current_ber = get_ber()
                dbg_msg(f"Results are in: BER = {current_ber}")

                result_bers.append(current_ber)

                for ber in self.ber_testers:
                    ber.reset()

            assert len(result_bers) == len(task.snrs)

            result = TaskResult(
                    task_id=task.task_id,
                    success=True,
                    message="Success!",
                    snrs=task.snrs,
                    bers=result_bers)

            self.results["task_id"] = result

            self._tasks.task_done()

    def get_ber(self):
        current_ber = np.array([np.array([ber.bit_errors,ber.finished_blocks]) for ber in self.ber_testers])
        current_ber = np.sum(current_ber[:,0]) / (code.k * np.sum(current_ber[:,1]))
        return current_ber

    def add_code(self, code):
        self.codes.append(code)

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

    def add_code(self, code: sdfec.ldpc_code):
        if not code.dec_OK:
            raise RuntimeError("Attempted to register code which cannot be decoded!")

        self.codes[code.hash] = code

