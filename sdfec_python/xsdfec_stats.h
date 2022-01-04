#pragma once

extern "C" {
#include "sdfec_usr_intf.h"
}

#include <memory>
#include <string>
#include <cstdint>

namespace xsdfec_stats_bindings {
std::shared_ptr<xsdfec_stats> make(uint32_t isr_err_count, uint32_t cecc_count,
                                   uint32_t uecc_count);

std::string str(const xsdfec_stats& self);
std::string repr(const xsdfec_stats& self);
std::string to_json(const xsdfec_stats& self);

} // namespace xsdfec_stats_bindings
