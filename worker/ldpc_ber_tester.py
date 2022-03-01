#!/usr/bin/python

import os
import math

class LDPCBERTester:
    """
    This class interfaces with the ldpc ber tester sysfs api
    """

    def __init__(self, path):
        if not os.path.isdir(path):
            raise RuntimeError(f"Cannot access sdfec, no such path: {path}")

        self.base_path = path

        # SDFEC control word values
        self.block_id = 0
        self.max_iterations = 8
        self.term_on_no_change = False
        self.term_on_pass = True
        self.include_parity_op = False
        self.hard_op = True
        self.code = 0

        # Initialize core information
        self.version = self._read("version")
        self.core_id = self._read("core_id")
        self.magic = self._read("magic")

        # Last failed array, initialized to be empty
        self.last_failed = []
        self._collect_errors = False

        # SNR
        self._snr = 0
        self._snr_scale = 1

        self._n = 0
        self._k = 0
        self._din_beats = 0

    @property
    def scratch(self):
        return self._read("scratch")

    @scratch.setter
    def scratch(self, val: int):
        assert isinstance(val, int)
        self._write("scratch", val)

    @property
    def enable(self):
        return bool(self._read("control_enable"))

    @enable.setter
    def enable(self, val: bool):
        assert isinstance(val, bool) or val in [0, 1]
        assert self._din_beats > 0

        if val:
            self._write("sdfec_ctrl_word", self._ctrl_word)

        self._write("control_enable", int(val))

    def _update_snr(self):
        sigma = self.snr_scale * 10 ** (-self.snr / 20)
        mu = -self._snr_scale

        # Clamping
        mu = min(max(mu, -7.5), 7.5)
        sigma = min(max(sigma, -10), 10)

        # Quantization
        offset = 0x00FF & int(round(mu    * 2**2)) # 6.2 int.fraction bits
        factor = 0xFFFF & int(round(sigma * 2**8)) # 8.8 int.fraction bits

        self._write("awgn_cfg_factor", factor)
        self._write("awgn_cfg_offset", offset)

    @property
    def snr(self):
        return self._snr

    @snr.setter
    def snr(self, val: float):
        self._snr = val
        self._update_snr()

    @property
    def snr_scale(self):
        return self._snr_scale

    @snr_scale.setter
    def snr_scale(self, val: float):
        assert 0 < val <= 10
        self._snr_scale = val
        self._update_snr()

    @property
    def n(self):
        return self._n

    @n.setter
    def n(self, val: int):
        self._n = val
        self._din_beats = (val // 16) + int(bool(val % 16))
        self._write("din_beats", self._din_beats)

    @property
    def k(self):
        return self._k

    @k.setter
    def k(self, val: int):
        self._k = val

        # One output lane of hard bits carries 128 values
        rem = val % 128
        if rem == 0:
            rem = 128


        self.last_mask = (1 << rem) - 1

        data = [hex(0xFFFFFFFF & (self.last_mask >> (i*32))) for i in range(4)]
        self._write("last_mask", " ".join(data))

    @property
    def finished_blocks(self):
        return self._read("finished_blocks")

    @property
    def bit_errors(self):
        return self._read("bit_errors")

    @property
    def in_flight(self):
        return self._read("in_flight")

    @property
    def iter_count(self):
        return self._read("iter_count")

    @property
    def failed_blocks(self):
        return self._read("failed_blocks")

    @property
    def int_status(self):
        return self._read("int_status")

    @property
    def int_enable(self):
        return self._read("int_enable")

    @int_enable.setter
    def int_enable(self, val: int):
        self._write("int_enable", int(val))

    @property
    def last_status(self):
        return self._read("last_status")

    @property
    def collect_errors(self):
        return self._collect_errors

    @collect_errors.setter
    def collect_errors(self, val: bool)
        self._collect_errors = bool(val)
        self._write("int_enable", int(val))

    def collect_last_failed(self, limit=1024):
        if not self._collect_errors:
            return []

        path = f"{self.base_path}/last_failed"
        vals = []

        while len(self.last_failed) + len(vals) < limit:
            with open(path, "r") as f:
                data = f.read().strip()

            if len(data) == 0:
                break

            lines = data.split("\n")
            for v in lines:
                vals.append(int(v))

            with open(path, "w") as f:
                f.write(f"{len(vals)}")

            if len(lines) < 32:
                break

        if len(self.last_failed) + len(vals) >= limit:
            self.collect_errors = False

        self.last_failed += vals

    def reset(self):
        self._write("control_resetn", 0)
        self.collect_last_failed()
        self.vals = []

    def _read(self, name):
        path = f"{self.base_path}/{name}"
        with open(path, "r") as f:
            data = f.read().strip()

        base = 10
        if data.startswith("0x"):
            data = data[2:]
            base = 16

        return int(data, base=base)

    def _write(self, name, val):
        path = f"{self.base_path}/{name}"
        with open(path, "w") as f:
            f.write(str(val))
            f.write("\n")

    @property
    def _ctrl_word(self):
        assert 0 <= self.block_id <= 255
        assert 0 <= self.max_iterations <= 63
        assert isinstance(self.term_on_no_change, bool)
        assert isinstance(self.term_on_pass, bool)
        assert isinstance(self.include_parity_op, bool)
        assert isinstance(self.hard_op, bool)
        assert 0 <= self.code <= 127

        val = 0

        val |= self.block_id                << 24 # to 32
        val |= self.max_iterations          << 18 # to 24
        val |= int(self.term_on_no_change)  << 17 # to 18
        val |= int(self.term_on_pass)       << 16 # to 17
        val |= int(self.include_parity_op)  << 15 # to 16
        val |= int(self.hard_op)            << 14 # to 15
        val |= self.code                    << 0  # to 8

        return 0xFFFFFFFF & val

