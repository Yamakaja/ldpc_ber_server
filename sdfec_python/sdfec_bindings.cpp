#include "sdfec.h"
#include "xsdfec_stats.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <boost/format.hpp>

namespace py = pybind11;

using namespace py::literals;

PYBIND11_MODULE(sdfec, m) {
    py::class_<ldpc_code>(m, "ldpc_code")
        .def(py::init())
        .def_property("n", &ldpc_code::get_n, &ldpc_code::set_n)
        .def_property("k", &ldpc_code::get_k, &ldpc_code::set_k)
        .def_property("p", &ldpc_code::get_psize, &ldpc_code::set_psize)
        .def_property("nlayers", &ldpc_code::get_nlayers, &ldpc_code::set_nlayers)
        .def_property("nqc", &ldpc_code::get_nqc, &ldpc_code::set_nqc)
        .def_property("nmqc", &ldpc_code::get_nmqc, &ldpc_code::set_nmqc)
        .def_property("nm", &ldpc_code::get_nm, &ldpc_code::set_nm)
        .def_property("norm_type", &ldpc_code::get_norm_type, &ldpc_code::set_norm_type)
        .def_property("no_packing", &ldpc_code::get_no_packing, &ldpc_code::set_no_packing)
        .def_property("special_qc", &ldpc_code::get_special_qc, &ldpc_code::set_special_qc)
        .def_property("no_final_parity", &ldpc_code::get_no_final_parity, &ldpc_code::set_no_final_parity)
        .def_property("max_schedule", &ldpc_code::get_max_schedule, &ldpc_code::set_max_schedule)
        .def_property("sc_table", &ldpc_code::get_sc_table, &ldpc_code::set_sc_table)
        .def_property("la_table", &ldpc_code::get_la_table, &ldpc_code::set_la_table)
        .def_property("qc_table", &ldpc_code::get_qc_table, &ldpc_code::set_qc_table)
        .def_property("dec_OK", &ldpc_code::get_dec_ok, &ldpc_code::set_dec_ok)
        .def_property("enc_OK", &ldpc_code::get_enc_ok, &ldpc_code::set_enc_ok)
        .def_property_readonly("json", &ldpc_code::to_json)
        .def_property_readonly("hash", &ldpc_code::get_hash)
        .def_property("dict", [](ldpc_code& self) {
                return py::dict(
                    "dec_OK"_a          = (uint32_t) self.get_dec_ok(),
                    "enc_OK"_a          = (uint32_t) self.get_enc_ok(),
                    "n"_a               = self.get_n(),
                    "k"_a               = self.get_k(),
                    "p"_a               = self.get_psize(),
                    "nlayers"_a         = self.get_nlayers(),
                    "nqc"_a             = self.get_nqc(),
                    "nmqc"_a            = self.get_nmqc(),
                    "nm"_a              = self.get_nm(),
                    "norm_type"_a       = self.get_norm_type(),
                    "no_packing"_a      = self.get_no_packing(),
                    "special_qc"_a      = self.get_special_qc(),
                    "no_final_parity"_a = self.get_no_final_parity(),
                    "max_schedule"_a    = self.get_max_schedule(),
                    "sc_table"_a        = self.get_sc_table(),
                    "la_table"_a        = self.get_la_table(),
                    "qc_table"_a        = self.get_qc_table()
                    );
        }, [](ldpc_code&, py::dict) {});

    py::class_<sdfec>(m, "sdfec")
        .def(py::init<int,bool>(), py::arg("id") = 0, py::arg("in_order") = true)
        .def("set_ldpc_code", &sdfec::set_ldpc_code)
        .def_property_readonly("id", &sdfec::get_id)
        .def_property_readonly("fd", &sdfec::get_fd)
        .def_property_readonly("in_order", &sdfec::is_in_order)
        .def("start", &sdfec::start)
        .def("stop", &sdfec::stop)
        .def_property_readonly("state", &sdfec::get_state)
        .def_property_readonly("active", &sdfec::is_active)
        .def_property("bypass", &sdfec::is_bypass, &sdfec::set_bypass)
        .def_property_readonly("stats", &sdfec::get_stats)
        .def_property_readonly("json", &sdfec::to_json)
        .def("clear_stats", &sdfec::clear_stats);

    py::class_<xsdfec_stats, std::shared_ptr<xsdfec_stats>>(m, "xsdfec_stats")
        .def(py::init(&xsdfec_stats_bindings::make), py::arg("isr_err_count"), py::arg("cecc_count"), py::arg("uecc_count"))
        .def_property_readonly("isr_err_count", [](const xsdfec_stats& self) { return self.isr_err_count; })
        .def_property_readonly("cecc_count", [](const xsdfec_stats& self) { return self.cecc_count; })
        .def_property_readonly("uecc_count", [](const xsdfec_stats& self) { return self.uecc_count; })
        .def("__str__", &xsdfec_stats_bindings::str)
        .def("__repr__", &xsdfec_stats_bindings::repr)
        .def_property_readonly("json", &xsdfec_stats_bindings::to_json)
        .def_property("dict", [](const xsdfec_stats& self) {
                return py::dict(
                        "isr_err_count"_a   = self.isr_err_count,
                        "cecc_count"_a      = self.cecc_count,
                        "uecc_count"_a      = self.uecc_count
                       );
                }, [](xsdfec_stats&, py::dict) {});

    py::enum_<xsdfec_state>(m, "xsdfec_state")
        .value("XSDFEC_INIT", xsdfec_state::XSDFEC_INIT)
        .value("XSDFEC_STARTED", xsdfec_state::XSDFEC_STARTED)
        .value("XSDFEC_STOPPED", xsdfec_state::XSDFEC_STOPPED)
        .value("XSDFEC_NEEDS_RESET", xsdfec_state::XSDFEC_NEEDS_RESET)
        .value("XSDFEC_PL_RECONFIGURE", xsdfec_state::XSDFEC_PL_RECONFIGURE);
}
