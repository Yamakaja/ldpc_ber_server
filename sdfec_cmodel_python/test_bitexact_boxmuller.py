#!/usr/bin/python

import numpy as np
import sdfec_cmodel

SNR = 10 * np.log10(1 / 0.5 ** 2)

model = sdfec_cmodel.sdfec_core("docsis_medium_decoder", "../test.yml")

ber = sdfec_cmodel.ldpc_ber_tester(0, model, 45)
ber.snr = SNR


maxdiff = 0
for blkid in range(128):
    rnd = ber.get_rnd_vector(blkid, 0)
    for i,(u0,u1,u2) in enumerate(rnd):
        a,b = sdfec_cmodel.bitexact_boxmuller(u0, u1, u2)
        u,v = sdfec_cmodel.boxmuller(u0, u1, u2)

        if abs(u-a) > maxdiff:
            maxdiff = abs(u-a)

        if abs(v-b) > maxdiff:
            maxdiff = abs(v-b)

        # print(i, u - a, v - b)

print("maxdiff:", maxdiff)
