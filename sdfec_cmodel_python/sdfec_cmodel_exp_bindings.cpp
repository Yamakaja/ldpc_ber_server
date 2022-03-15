#include "sdfec_cmodel_exp.h"

#include <pybind11/buffer_info.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <fmt/core.h>

#include <array>
#include <cstring>

namespace py = pybind11;

using namespace py::literals;

#define STR(s) #s

PYBIND11_MODULE(sdfec_cmodel_exp, m)
{
    sdfec_cmodel_exp::init();
    m.def("gen_ldpc_params", &sdfec_cmodel_exp::gen_ldpc_params, "path"_a);
};
