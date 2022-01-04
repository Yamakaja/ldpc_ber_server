#include "xsdfec_stats.h"
#include <boost/format.hpp>

namespace xsdfec_stats_bindings {
std::shared_ptr<xsdfec_stats> make(uint32_t isr_err_count, uint32_t cecc_count,
                                   uint32_t uecc_count) {
    std::shared_ptr<xsdfec_stats> ptr = std::make_shared<xsdfec_stats>();
    ptr->isr_err_count = isr_err_count;
    ptr->cecc_count = cecc_count;
    ptr->uecc_count = uecc_count;

    return ptr;
}

std::string str(const xsdfec_stats& self) {
    return boost::str(boost::format("{isr_err_count: %u, cecc_count: %u, uecc_count: %u}")
            % self.isr_err_count
            % self.cecc_count
            % self.uecc_count);
}

std::string repr(const xsdfec_stats& self) {
    return boost::str(boost::format("xsdfec_stats(isr_err_count=%u, cecc_count=%u, uecc_count=%u)")
            % self.isr_err_count
            % self.cecc_count
            % self.uecc_count);
}

std::string to_json(const xsdfec_stats& self) {
    return boost::str(boost::format(R"JS({
    "isr_err_count": %u,
    "cecc_count": %u,
    "uecc_count": %u
})JS") % self.isr_err_count % self.cecc_count % self.uecc_count);
}

}
