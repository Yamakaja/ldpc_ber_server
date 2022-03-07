#include "sdfec_cmodel.h"
#include "xoroshiro128plus.h"

#include <pybind11/buffer_info.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <boost/format.hpp>

#include <array>
#include <cstring>

namespace py = pybind11;
using sdfec_cmodel::ldpc_parameter_wrapper;
using sdfec_cmodel::sdfec_core;

using namespace py::literals;

#define STR(s) #s

namespace {
#define NUMPY_TO_XIP_ARR(t)                                                                                           \
    std::shared_ptr<xip_array_##t> numpy_to_xip_array_##t(                                                            \
        const py::array_t<xip_##t, py::array::f_style | py::array::forcecast>& din)                                   \
    {                                                                                                                 \
        auto array = std::shared_ptr<xip_array_##t>(xip_array_##t##_create(), sdfec_cmodel::xip_array_deleter_##t()); \
        auto ndim = din.ndim();                                                                                       \
                                                                                                                      \
        xip_array_##t##_reserve_dim(array.get(), ndim);                                                               \
        xip_array_##t##_reserve_data(array.get(), din.size());                                                        \
                                                                                                                      \
        array->data_size = din.size();                                                                                \
        array->data_capacity = din.size();                                                                            \
                                                                                                                      \
        std::memcpy(array->data, din.data(), din.nbytes());                                                           \
                                                                                                                      \
        for (int i = 0; i < ndim; i++)                                                                                \
            array->dim[i] = din.shape(i);                                                                             \
                                                                                                                      \
        array->dim_size = ndim;                                                                                       \
        array->dim_capacity = ndim;                                                                                   \
                                                                                                                      \
        return array;                                                                                                 \
    }

NUMPY_TO_XIP_ARR(real);
NUMPY_TO_XIP_ARR(bit);
} // namespace
PYBIND11_MODULE(sdfec_cmodel, m)
{

    py::class_<ldpc_parameter_wrapper, std::shared_ptr<ldpc_parameter_wrapper>>(m, "sdfec_ldpc_params")
#define RO_LDPC_ATTRIB(x) def_property_readonly(STR(x), [](ldpc_parameter_wrapper* self) { return self->params.x; })
        .RO_LDPC_ATTRIB(n)
        .RO_LDPC_ATTRIB(k)
        .RO_LDPC_ATTRIB(psize)
        .RO_LDPC_ATTRIB(nlayers)
        .RO_LDPC_ATTRIB(nqc)
        .RO_LDPC_ATTRIB(nmqc)
        .RO_LDPC_ATTRIB(nm)
        .RO_LDPC_ATTRIB(norm_type)
        .RO_LDPC_ATTRIB(no_packing)
        .RO_LDPC_ATTRIB(special_qc)
        .RO_LDPC_ATTRIB(no_final_parity)
        .RO_LDPC_ATTRIB(max_schedule)
#undef RO_LDPC_ATTRIB
        .def_property_readonly("sc_table", &sdfec_cmodel::get_sc_table)
        .def_property_readonly("la_table", &sdfec_cmodel::get_la_table)
        .def_property_readonly("qc_table", &sdfec_cmodel::get_qc_table)
        .def_property_readonly("dict",
                               [](std::shared_ptr<ldpc_parameter_wrapper> self) {
                                   return py::dict("enc_OK"_a = self->enc_OK,
                                                   "dec_OK"_a = self->dec_OK,
                                                   "n"_a = self->params.n,
                                                   "k"_a = self->params.k,
                                                   "psize"_a = self->params.psize,
                                                   "nlayers"_a = self->params.nlayers,
                                                   "nqc"_a = self->params.nqc,
                                                   "nmqc"_a = self->params.nmqc,
                                                   "nm"_a = self->params.nm,
                                                   "norm_type"_a = self->params.norm_type,
                                                   "no_packing"_a = self->params.no_packing,
                                                   "special_qc"_a = self->params.special_qc,
                                                   "no_final_parity"_a = self->params.no_final_parity,
                                                   "max_schedule"_a = self->params.max_schedule,
                                                   "sc_table"_a = sdfec_cmodel::get_sc_table(self),
                                                   "la_table"_a = sdfec_cmodel::get_la_table(self),
                                                   "qc_table"_a = sdfec_cmodel::get_qc_table(self));
                               })
        .def_property_readonly("raw_sc_table",
                               [](std::shared_ptr<ldpc_parameter_wrapper> self) {
                                   return py::array_t<uint32_t>(
                                       py::buffer_info((uint32_t*)self->params.sc_table,
                                                       std::vector<size_t>({ self->params.nlayers }),
                                                       std::vector<size_t>({ sizeof(uint32_t) })));
                               })
        .def_property_readonly("raw_la_table",
                               [](std::shared_ptr<ldpc_parameter_wrapper> self) {
                                   return py::array_t<uint32_t>(py::buffer_info(
                                       (uint32_t*)self->params.la_table,
                                       std::vector<size_t>({ self->params.nlayers, 3 }),
                                       std::vector<size_t>({ 3 * sizeof(uint32_t), sizeof(uint32_t) })));
                               })
        .def_property_readonly("raw_qc_table", [](std::shared_ptr<ldpc_parameter_wrapper> self) {
            return py::array_t<uint32_t>(
                py::buffer_info((uint32_t*)self->params.qc_table,
                                std::vector<size_t>({ self->params.nqc, 4 }),
                                std::vector<size_t>({ 4 * sizeof(uint32_t), sizeof(uint32_t) })));
        });

    m.def("gen_ldpc_params", &sdfec_cmodel::gen_ldpc_params, "src_file"_a);

    py::enum_<sdfec_cmodel::ldpc_standard>(m, "ldpc_standard")
        .value("CUSTOM", sdfec_cmodel::ldpc_standard::CUSTOM)
        .value("DECODE_5G", sdfec_cmodel::ldpc_standard::DECODE_5G)
        .value("ENCODE_5G", sdfec_cmodel::ldpc_standard::ENCODE_5G)
        .value("DECODE_WIFI_802_11", sdfec_cmodel::ldpc_standard::DECODE_WIFI_802_11)
        .value("ENCODE_WIFI_802_11", sdfec_cmodel::ldpc_standard::ENCODE_WIFI_802_11)
        .value("DECODE_DOCSIS_3_1", sdfec_cmodel::ldpc_standard::DECODE_DOCSIS_3_1)
        .value("ENCODE_DOCSIS_3_1", sdfec_cmodel::ldpc_standard::ENCODE_DOCSIS_3_1);

    py::enum_<sdfec_cmodel::quant_mode>(m, "quant_mode")
        .value("SATURATE", sdfec_cmodel::quant_mode::SATURATE)
        .value("WRAP", sdfec_cmodel::quant_mode::WRAP);

    py::enum_<sdfec_cmodel::fec_mode>(m, "fec_mode")
        .value("LDPC", sdfec_cmodel::fec_mode::LDPC)
        .value("TURBO", sdfec_cmodel::fec_mode::TURBO);

    py::class_<XIP_LDPC_CTRL, std::shared_ptr<XIP_LDPC_CTRL>>(m, "ctrl_packet")
        .def(py::init(&sdfec_cmodel::make_ctrl_packet),
             "code"_a = 0,
             "op"_a = 0,
             "z_j"_a = 0,
             "z_set"_a = 0,
             "bg"_a = 0,
             "sc_idx"_a = 0,
             "mb"_a = 0,
             "hard_op"_a = 0,
             "include_parity_op"_a = 0,
             "crc_type"_a = 0,
             "term_on_pass"_a = 0,
             "term_on_no_change"_a = 0,
             "max_iterations"_a = 0,
             "id"_a = 0)
        .def_readwrite("code", &XIP_LDPC_CTRL::code)
        .def_readwrite("op", &XIP_LDPC_CTRL::op)
        .def_readwrite("z_j", &XIP_LDPC_CTRL::z_j)
        .def_readwrite("z_set", &XIP_LDPC_CTRL::z_set)
        .def_readwrite("bg", &XIP_LDPC_CTRL::bg)
        .def_readwrite("sc_idx", &XIP_LDPC_CTRL::sc_idx)
        .def_readwrite("mb", &XIP_LDPC_CTRL::mb)
        .def_readwrite("hard_op", &XIP_LDPC_CTRL::hard_op)
        .def_readwrite("include_parity_op", &XIP_LDPC_CTRL::include_parity_op)
        .def_readwrite("crc_type", &XIP_LDPC_CTRL::crc_type)
        .def_readwrite("term_on_pass", &XIP_LDPC_CTRL::term_on_pass)
        .def_readwrite("term_on_no_change", &XIP_LDPC_CTRL::term_on_no_change)
        .def_readwrite("max_iterations", &XIP_LDPC_CTRL::max_iterations)
        .def_readwrite("id", &XIP_LDPC_CTRL::id)
        .def("__str__", &sdfec_cmodel::ctrl_packet_to_string)
        .def("__repr__", &sdfec_cmodel::ctrl_packet_to_string);

    py::class_<XIP_LDPC_STAT, std::shared_ptr<XIP_LDPC_STAT>>(m, "status_packet")
        .def(py::init(&sdfec_cmodel::make_status_packet),
             "code"_a = 0,
             "op"_a = 0,
             "crc_type"_a = 0,
             "z_j"_a = 0,
             "z_set"_a = 0,
             "bg"_a = 0,
             "mb"_a = 0,
             "hard_op"_a = 0,
             "passed"_a = 0,
             "term_pass"_a = 0,
             "term_no_change"_a = 0,
             "dec_iter"_a = 0,
             "id"_a = 0)
        .def_readwrite("code", &XIP_LDPC_STAT::code)
        .def_readwrite("op", &XIP_LDPC_STAT::op)
        .def_readwrite("crc_type", &XIP_LDPC_STAT::crc_type)
        .def_readwrite("z_j", &XIP_LDPC_STAT::z_j)
        .def_readwrite("z_set", &XIP_LDPC_STAT::z_set)
        .def_readwrite("bg", &XIP_LDPC_STAT::bg)
        .def_readwrite("mb", &XIP_LDPC_STAT::mb)
        .def_readwrite("hard_op", &XIP_LDPC_STAT::hard_op)
        .def_readwrite("passed", &XIP_LDPC_STAT::pass)
        .def_readwrite("term_pass", &XIP_LDPC_STAT::term_pass)
        .def_readwrite("term_no_change", &XIP_LDPC_STAT::term_no_change)
        .def_readwrite("dec_iter", &XIP_LDPC_STAT::dec_iter)
        .def_readwrite("id", &XIP_LDPC_STAT::id)
        .def("__str__", &sdfec_cmodel::status_packet_to_string)
        .def("__repr__", &sdfec_cmodel::status_packet_to_string);

    py::class_<sdfec_core, std::shared_ptr<sdfec_core>>(m, "sdfec_core")
        .def(py::init<std::string,
                      std::string,
                      sdfec_cmodel::fec_mode,
                      sdfec_cmodel::ldpc_standard,
                      int,
                      bool,
                      sdfec_cmodel::quant_mode>(),
             "name"_a,
             "code_src"_a,
             "mode"_a = sdfec_cmodel::fec_mode::LDPC,
             "standard"_a = sdfec_cmodel::ldpc_standard::CUSTOM,
             "scale_override"_a = -1,
             "bypass"_a = false,
             "quantization_mode"_a = sdfec_cmodel::quant_mode::SATURATE)
        .def("reset", &sdfec_core::reset)
        .def("add_ldpc_code", &sdfec_core::add_ldpc_code, "number"_a, "code"_a)
        .def("set_turbo_params", &sdfec_core::set_turbo_params, "alg"_a, "scale"_a)
        .def_property_readonly("num_loaded_ldpc_codes", &sdfec_core::get_num_loaded_ldpc_codes)
        .def(
            "process",
            [](std::shared_ptr<sdfec_core> self,
               std::shared_ptr<XIP_LDPC_CTRL> ctrl,
               py::array_t<xip_real, py::array::f_style | py::array::forcecast>& data) {
                return self->process(ctrl, numpy_to_xip_array_real(data));
            },
            "ctrl"_a,
            "data"_a);

#define XIP_ARRAY_BINDINGS(t)                                                                            \
    py::class_<xip_array_##t, std::shared_ptr<xip_array_##t>>(m, "array_" STR(t), py::buffer_protocol()) \
        .def(py::init(&numpy_to_xip_array_##t), "data"_a)                                                \
        .def_buffer([](xip_array_##t* self) {                                                            \
            std::vector<size_t> strides(self->dim_size);                                                 \
                                                                                                         \
            strides[0] = sizeof(xip_##t);                                                                \
            for (uint32_t i = 1; i < self->dim_size; i++)                                                \
                strides[i] = strides[i - 1] * self->dim[i - 1];                                          \
                                                                                                         \
            return py::buffer_info(                                                                      \
                self->data, std::vector<size_t>(&self->dim[0], &self->dim[self->dim_size]), strides);    \
        });

    XIP_ARRAY_BINDINGS(real);
    XIP_ARRAY_BINDINGS(bit);

    py::class_<xoroshiro128plus_t, std::shared_ptr<xoroshiro128plus_t>>(m, "xoroshiro128plus")
        .def(py::init([](uint64_t seed) {
                 auto xoro = std::make_shared<xoroshiro128plus_t>();
                 xoroshiro128plus_init(xoro.get(), seed);
                 return xoro;
             }),
             "seed"_a)
        .def(py::init([](uint64_t s_0, uint64_t s_1) {
                 auto xoro = std::make_shared<xoroshiro128plus_t>();
                 xoro->s[0] = s_0;
                 xoro->s[1] = s_1;
                 return xoro;
             }),
             "s_0"_a,
             "s_1"_a)
        .def("next", [](std::shared_ptr<xoroshiro128plus_t> self) { return xoroshiro128plus_next(self.get()); })
        .def(
            "forward",
            [](std::shared_ptr<xoroshiro128plus_t> self, uint64_t steps) {
                return xoroshiro128plus_forward(self.get(), steps);
            },
            "steps"_a)
        .def("jump", [](std::shared_ptr<xoroshiro128plus_t> self) { xoroshiro128plus_jump(self.get()); })
        .def_property_readonly("s", [](std::shared_ptr<xoroshiro128plus_t> self) {
            return std::array<uint64_t, 2>({ self->s[0], self->s[1] });
        });
};
