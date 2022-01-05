from worker import Worker
import sdfec
import yaml

def run(config, debug):
    worker = Worker(config, debug)

    with open("test.sdfec.yml", "r") as f:
        code = worker.add_code(yaml.load(f))

    task = SimulationTask(0, code, [10], term_time=10)

    worker.submit_task(task)
