from worker import Worker,SimulationTask
import sdfec
import yaml
import numpy as np

def run(config, debug):
    worker = Worker(config, debug)

    with open("test.sdfec.yml", "r") as f:
        code = worker.add_code(yaml.load(f))

    points = (7-3)*4+1
    task = SimulationTask(0, code, np.linspace(3, 7, points), snr_scales=np.ones(points)*2, term_time=10, term_errors=1e3, max_iterations=32)
    worker.submit_task(task)

    worker.join()
    print(worker.results)
    exit(0)

