#include "sdfec_cmodel_exp.h"

#include <dlfcn.h>
#include <fmt/core.h>
#include <iostream>

namespace sdfec_cmodel_exp {

namespace {
void* sdfec_handle = nullptr;

struct {
    xip_uint (*gen_ldpc_params)(const char* spec,
                                xip_sd_fec_v1_1_ldpc_parameters* params,
                                xip_msg_handler msg_handler,
                                void* msg_handle);
} sdfec_ops{ nullptr };

void xip_msg_callback(void* /*handle*/, int /* error */, const char* msg) { std::cerr << msg << std::endl; }
}; // namespace

void init()
{
    dlerror();
    sdfec_handle = dlmopen(LM_ID_NEWLM, SDFEC_CMODEL_DIR "/libIp_sd_fec_v1_1_bitacc_cmodel.so", RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND);
    if (sdfec_handle == nullptr) {
        throw std::runtime_error(fmt::format("Failed to open libIp_sd_fec_v1_1_bitacc_cmodel.so: {}", dlerror()));
    }

    sdfec_ops.gen_ldpc_params =
        reinterpret_cast<decltype(sdfec_ops.gen_ldpc_params)>(dlsym(sdfec_handle, "xip_sd_fec_v1_1_gen_ldpc_params"));
    if (sdfec_ops.gen_ldpc_params == nullptr)
        throw std::runtime_error(
            "Failed to acquire 'xip_sd_fec_v1_1_gen_ldpc_params' from libIp_sd_fec_v1_1_bitacc_cmodel.so");
}

unsigned int gen_ldpc_params(char* path)
{
    xip_sd_fec_v1_1_ldpc_parameters params{};

    sdfec_ops.gen_ldpc_params(path, &params, &xip_msg_callback, nullptr);

    return params.n;
}
} // namespace sdfec_cmodel_exp
