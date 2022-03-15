#include "sdfec_cmodel_shim.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include "sd_fec_v1_1_bitacc_cmodel.h"
#pragma GCC diagnostic pop

#include <fmt/core.h>

#include <dlfcn.h>
#include <cerrno>
#include <cstdlib>
#include <iostream>

#define STR(x) #x
#define XSTR(x) STR(x)

#define XIP_TYPEARGS(...) __VA_ARGS__
#define XIP_ARGS(...) __VA_ARGS__

#define XIP_SD(x) xip_sd_fec_v1_1##x

#define XIP_SHIM(ret, name, TYPEARGS, ARGS)             \
    namespace sdfec_cmodel::shim {                      \
    ret (*xip_##name)(XIP_TYPEARGS TYPEARGS) = nullptr; \
    }                                                   \
    extern "C" ret name(XIP_TYPEARGS TYPEARGS) { return sdfec_cmodel::shim::xip_##name(XIP_ARGS ARGS); }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"

#define XIP_SHIM_ARRAY(TARGET, TYPE)                                                                                  \
    XIP_SHIM(xip_array_##TYPE* const, xip_##TARGET##_xip_array_##TYPE##_create, (), ());                              \
    XIP_SHIM(xip_array_##TYPE* const,                                                                                 \
             xip_##TARGET##_xip_array_##TYPE##_create_ex,                                                             \
             (const xip_array_##TYPE##_operations* ops),                                                              \
             (ops));                                                                                                  \
    XIP_SHIM(const xip_status,                                                                                        \
             xip_##TARGET##_xip_array_##TYPE##_reserve_data,                                                          \
             (xip_array_##TYPE * p, size_t max_nels),                                                                 \
             (p, max_nels));                                                                                          \
    XIP_SHIM(const xip_status,                                                                                        \
             xip_##TARGET##_xip_array_##TYPE##_reserve_dim,                                                           \
             (xip_array_##TYPE * p, size_t max_ndim),                                                                 \
             (p, max_ndim));                                                                                          \
    XIP_SHIM(xip_array_##TYPE* const, xip_##TARGET##_xip_array_##TYPE##_destroy, (xip_array_##TYPE * p), (p));        \
    XIP_SHIM(                                                                                                         \
        const size_t, xip_##TARGET##_xip_array_##TYPE##_sub2ind_1d, (const xip_array_##TYPE* p, size_t s0), (p, s0)); \
    XIP_SHIM(const size_t,                                                                                            \
             xip_##TARGET##_xip_array_##TYPE##_sub2ind_2d,                                                            \
             (const xip_array_##TYPE* p, size_t s0, size_t s1),                                                       \
             (p, s0, s1));                                                                                            \
    XIP_SHIM(const size_t,                                                                                            \
             xip_##TARGET##_xip_array_##TYPE##_sub2ind_3d,                                                            \
             (const xip_array_##TYPE* p, size_t s0, size_t s1, size_t s2),                                            \
             (p, s0, s1, s2));                                                                                        \
    XIP_SHIM(const size_t,                                                                                            \
             xip_##TARGET##_xip_array_##TYPE##_sub2ind_4d,                                                            \
             (const xip_array_##TYPE* p, size_t s0, size_t s1, size_t s2, size_t s3),                                 \
             (p, s0, s1, s2, s3));                                                                                    \
    XIP_SHIM(const xip_status,                                                                                        \
             xip_##TARGET##_xip_array_##TYPE##_set_element,                                                           \
             (xip_array_##TYPE * p, const xip_##TYPE* value, size_t index),                                           \
             (p, value, index));                                                                                      \
    XIP_SHIM(const xip_status,                                                                                        \
             xip_##TARGET##_xip_array_##TYPE##_get_element,                                                           \
             (const xip_array_##TYPE* p, xip_##TYPE* value, size_t index),                                            \
             (p, value, index));

XIP_SHIM_ARRAY(sd_fec_v1_1, real);
XIP_SHIM_ARRAY(sd_fec_v1_1, bit);

#pragma GCC diagnostic pop

#define XIP_SD_SHIM(ret, name, TYPEARGS, ARGS)                   \
    namespace sdfec_cmodel::shim {                               \
    ret (*sd_##name)(XIP_TYPEARGS TYPEARGS) = nullptr;           \
    }                                                            \
    extern "C" ret xip_sd_fec_v1_1_##name(XIP_TYPEARGS TYPEARGS) \
    {                                                            \
        return sdfec_cmodel::shim::sd_##name(XIP_ARGS ARGS);     \
    }

XIP_SD_SHIM(const char*, get_version, (void), ());
XIP_SD_SHIM(XIP_SD() *,
            create,
            (const XIP_SD(_config) * config, xip_msg_handler handler, void* handle),
            (config, handler, handle));
XIP_SD_SHIM(xip_status, reset, (XIP_SD() * model), (model));
XIP_SD_SHIM(xip_status, set_config_params, (XIP_SD() * model, const XIP_SD(_config) * params), (model, params));
XIP_SD_SHIM(xip_status,
            gen_ldpc_spec,
            (const xip_array_real* H, xip_uint psize, const char* spec, xip_msg_handler msg_handler, void* msg_handle),
            (H, psize, spec, msg_handler, msg_handle));
XIP_SD_SHIM(xip_status,
            gen_parity_check_mat,
            (const char* spec, xip_array_real* H, xip_msg_handler msg_handler, void* msg_handle),
            (spec, H, msg_handler, msg_handle));
XIP_SD_SHIM(xip_uint,
            gen_ldpc_params,
            (const char* spec, XIP_SD(_ldpc_parameters) * params, xip_msg_handler msg_handler, void* msg_handle),
            (spec, params, msg_handler, msg_handle));
XIP_SD_SHIM(xip_status,
            destroy_ldpc_params,
            (XIP_SD(_ldpc_parameters) * params, xip_msg_handler msg_handler, void* msg_handle),
            (params, msg_handler, msg_handle));
XIP_SD_SHIM(xip_status,
            add_ldpc_params,
            (XIP_SD() * model, xip_uint code_num, const XIP_SD(_ldpc_parameters) * params),
            (model, code_num, params));
XIP_SD_SHIM(xip_status, set_turbo_params, (XIP_SD() * model, const XIP_SD(_td_parameters) * params), (model, params));
XIP_SD_SHIM(int, get_num_ldpc_codes, (XIP_SD() * model), (model));
XIP_SD_SHIM(xip_status,
            get_ldpc_params,
            (XIP_SD() * model, const XIP_SD(_ctrl_packet) * ctrl, XIP_SD(_ldpc_parameters) * params),
            (model, ctrl, params));
XIP_SD_SHIM(xip_status,
            process,
            (XIP_SD() * model,
             const XIP_SD(_ctrl_packet) * ctrl,
             const xip_array_real* data,
             xip_array_bit* hard,
             xip_array_real* soft,
             XIP_SD(_stat_packet) * stat),
            (model, ctrl, data, hard, soft, stat));
XIP_SD_SHIM(xip_status, destroy, (XIP_SD() * model), (model));


namespace sdfec_cmodel {
namespace shim {

namespace {
void* sdfec_handle = NULL;
}

void init()
{
    dlerror();
    sdfec_handle = dlmopen(
        LM_ID_NEWLM, SDFEC_CMODEL_DIR "/libIp_sd_fec_v1_1_bitacc_cmodel.so", RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND);

    if (!sdfec_handle)
        throw std::runtime_error(
            fmt::format("Failed to open shared library libIp_sd_fec_v1_1_bitacc_cmodel.so: {}", dlerror()));

#undef XIP_SHIM
#define XIP_SHIM(x)                                                              \
    xip_##x = reinterpret_cast<decltype(xip_##x)>(dlsym(sdfec_handle, XSTR(x))); \
    if (xip_##x == nullptr)                                                      \
        throw std::runtime_error(                                                \
            fmt::format("Failed to acquire " XSTR(x) " from libIp_sd_fec_v1_1_bitacc_cmodel.so: {}", dlerror()));

#undef XIP_SHIM_ARRAY
#define XIP_SHIM_ARRAY(TARGET, TYPE)                          \
    XIP_SHIM(xip_##TARGET##_xip_array_##TYPE##_create);       \
    XIP_SHIM(xip_##TARGET##_xip_array_##TYPE##_create_ex);    \
    XIP_SHIM(xip_##TARGET##_xip_array_##TYPE##_reserve_data); \
    XIP_SHIM(xip_##TARGET##_xip_array_##TYPE##_reserve_dim);  \
    XIP_SHIM(xip_##TARGET##_xip_array_##TYPE##_destroy);      \
    XIP_SHIM(xip_##TARGET##_xip_array_##TYPE##_sub2ind_1d);   \
    XIP_SHIM(xip_##TARGET##_xip_array_##TYPE##_sub2ind_2d);   \
    XIP_SHIM(xip_##TARGET##_xip_array_##TYPE##_sub2ind_3d);   \
    XIP_SHIM(xip_##TARGET##_xip_array_##TYPE##_sub2ind_4d);   \
    XIP_SHIM(xip_##TARGET##_xip_array_##TYPE##_set_element);  \
    XIP_SHIM(xip_##TARGET##_xip_array_##TYPE##_get_element);

    XIP_SHIM_ARRAY(sd_fec_v1_1, real);
    XIP_SHIM_ARRAY(sd_fec_v1_1, bit);

#undef XIP_SD_SHIM
#define XIP_SD_SHIM(x)                                                         \
    sd_##x = reinterpret_cast<decltype(sd_##x)>(dlsym(sdfec_handle, XSTR(xip_sd_fec_v1_1_##x))); \
    if (sd_##x == nullptr)                                                     \
        throw std::runtime_error(                                              \
            fmt::format("Failed to acquire " XSTR(x) " from libIp_sd_fec_v1_1_bitacc_cmodel.so: {}", dlerror()));

    XIP_SD_SHIM(get_version);
    XIP_SD_SHIM(create);
    XIP_SD_SHIM(reset);
    XIP_SD_SHIM(set_config_params);
    XIP_SD_SHIM(gen_ldpc_spec);
    XIP_SD_SHIM(gen_parity_check_mat);
    XIP_SD_SHIM(gen_ldpc_params);
    XIP_SD_SHIM(destroy_ldpc_params);
    XIP_SD_SHIM(add_ldpc_params);
    XIP_SD_SHIM(set_turbo_params);
    XIP_SD_SHIM(get_num_ldpc_codes);
    XIP_SD_SHIM(get_ldpc_params);
    XIP_SD_SHIM(process);
    XIP_SD_SHIM(destroy);
}

} // namespace shim
} // namespace sdfec_cmodel
