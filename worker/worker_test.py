from worker import Worker,SimulationTask
import sdfec
import yaml
import numpy as np

def run(config, debug):
    worker = Worker(config, debug)

    with open("test.sdfec.yml", "r") as f:
        code = worker.add_code(yaml.load(f))

    a = 6
    b = 6.5
    N = 9
    points = 11
    task = SimulationTask(0, code, np.linspace(a, b, points), snr_scales=np.ones(points)*2, term_time=120, term_errors=1e5, max_iterations=32)
    worker.submit_task(task)

    worker.join()
    print(worker.results)
    exit(0)

