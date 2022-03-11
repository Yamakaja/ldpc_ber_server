#!/usr/bin/python

import numpy as np
import sdfec_cmodel

SNR = 10 * np.log10(1 / 0.5 ** 2)

model = sdfec_cmodel.sdfec_core("docsis_medium_decoder", "../test.yml")

ber = sdfec_cmodel.ldpc_ber_tester(0, model, 45)
ber.snr = SNR

src,status,dst_sd,dst_hd = ber.simulate_block(0, max_iterations=16, term_on_no_change=True)
