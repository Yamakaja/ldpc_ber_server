from worker import Worker,SimulationTask
import sdfec
import yaml
import numpy as np

def run(config, debug):
    worker = Worker(config, debug)

    with open("test.sdfec.yml", "r") as f:
        code = worker.add_code(yaml.load(f))

    a = 3
    b = 7
    N = 4
    points = (b-a)*N + 1
    task = SimulationTask(code, np.linspace(a, b, points), snr_scales=np.ones(points)*2, term_time=10, term_errors=1e5, max_iterations=32)
    worker.submit_task(task)

    worker.join()
    print(worker.results)
    exit(0)

