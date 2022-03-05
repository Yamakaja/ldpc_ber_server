#!/usr/bin/python3
import numpy as np
import build.sdfec_cmodel as sdfec_cmodel

core = sdfec_cmodel.sdfec_core("test_core", "test.yml")
code = sdfec_cmodel.gen_ldpc_params("test.yml")

SNR = 6
sigma = 10**(-SNR/20)
data = np.random.randn(code.n) * sigma - 1

ctrl = sdfec_cmodel.ctrl_packet(code=0,
        op=0,
        hard_op=1,
        include_parity_op=False,
        term_on_pass=True,
        max_iterations=32,
        id=0)

hd_raw, _, status = core.process(ctrl, data)

hd = np.array(hd_raw, copy=False)

vals, counts = np.unique(hd, return_counts=True)
print("Value distribution ([0, 1]):", counts / code.k)
print(status)

if status.passed:
    print("Passed!");
else:
    print("Block failed to decode!")
