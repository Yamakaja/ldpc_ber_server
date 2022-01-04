#!/usr/bin/python

import json
import sdfec
from worker import TaskResult

from flask import Response

class SDFECJsonEncoder(json.JSONEncoder):
    def default(self, o):
        if type(o) in [sdfec.xsdfec_stats, sdfec.ldpc_code, sdfec.sdfec]:
            return o.dict
        if type(o) in [TaskResult,SimulationTask]:
            return o.__dict__
        return json.JSONEncoder.default(self, o)

def jsonify(o, code=200):
    d = json.dumps(o, cls=SDFECJsonEncoder)
    return Response(d, status=code, mimetype="application/json")
