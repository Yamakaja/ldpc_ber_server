#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include "sd_fec_v1_1_bitacc_cmodel.h"
#pragma GCC diagnostic pop

#include <memory>
#include <string>
#include <vector>

#define XIP_SDFEC(x) xip_sd_fec_v1_1_##x
#define XIP_LDPC_PARAMS XIP_SDFEC(ldpc_parameters)
#define XIP_LDPC_CTRL XIP_SDFEC(ctrl_packet)
#define XIP_LDPC_STAT XIP_SDFEC(stat_packet)
#define XIP_TURBO_DEC_PARAM XIP_SDFEC(td_parameters)

namespace sdfec_cmodel {

#define XIP_ARRAY_DELETER(t)                                                     \
    struct xip_array_deleter_##t {                                               \
        void operator()(xip_array_##t* v) const { xip_array_##t##_destroy(v); }; \
    };

XIP_ARRAY_DELETER(real);
XIP_ARRAY_DELETER(bit);

class ldpc_parameter_wrapper
{
public:
    XIP_LDPC_PARAMS params{};
    bool enc_OK = false;
    bool dec_OK = false;

    ldpc_parameter_wrapper() = default;
    virtual ~ldpc_parameter_wrapper();
};

std::shared_ptr<ldpc_parameter_wrapper> gen_ldpc_params(char* src_file);

std::vector<uint32_t> get_sc_table(const std::shared_ptr<ldpc_parameter_wrapper> self);
std::vector<uint32_t> get_la_table(const std::shared_ptr<ldpc_parameter_wrapper> self);
std::vector<uint32_t> get_qc_table(const std::shared_ptr<ldpc_parameter_wrapper> self);

enum class ldpc_standard {
    CUSTOM = 0,
    DECODE_5G = 1,
    ENCODE_5G = 2,
    DECODE_WIFI_802_11 = 3,
    ENCODE_WIFI_802_11 = 4,
    DECODE_DOCSIS_3_1 = 5,
    ENCODE_DOCSIS_3_1 = 6
};

enum class quant_mode { SATURATE = 0, WRAP = 1 };

enum class fec_mode { LDPC = XIP_SD_FEC_v1_1_PARAM_FEC_LDPC, TURBO = XIP_SD_FEC_v1_1_PARAM_FEC_TURBO };

struct sdfec_core {
    xip_sd_fec_v1_1* m_fec_handle;
    xip_sd_fec_v1_1_config m_config;
    const std::string m_name;
    const std::string m_code_file;
    const std::string m_version;

    sdfec_core(std::string name,
               std::string code_src,
               fec_mode mode = fec_mode::LDPC,
               ldpc_standard standard = ldpc_standard::CUSTOM,
               int scale_override = -1,
               bool bypass = false,
               quant_mode quantization_mode = quant_mode::SATURATE);

    virtual ~sdfec_core();

    void reset();

    void add_ldpc_code(int number, std::shared_ptr<ldpc_parameter_wrapper> code);

    void set_turbo_params(uint32_t alg, uint32_t scale);

    int get_num_loaded_ldpc_codes();

    std::tuple<std::shared_ptr<xip_array_bit>, std::shared_ptr<xip_array_real>, std::shared_ptr<XIP_LDPC_STAT>>
    process(std::shared_ptr<XIP_LDPC_CTRL> ctrl, std::shared_ptr<xip_array_real> data);
};

std::string ctrl_packet_to_string(const std::shared_ptr<XIP_LDPC_CTRL> self);
std::string status_packet_to_string(const std::shared_ptr<XIP_LDPC_STAT> self);

std::shared_ptr<XIP_LDPC_CTRL> make_ctrl_packet(xip_uint code = 0,
                                                xip_uint op = 0,
                                                xip_uint z_j = 0,
                                                xip_uint z_set = 0,
                                                xip_uint bg = 0,
                                                xip_uint sc_idx = 0,
                                                xip_uint mb = 0,
                                                xip_bit hard_op = 0,
                                                xip_bit include_parity_op = 0,
                                                xip_bit crc_type = 0,
                                                xip_bit term_on_pass = 0,
                                                xip_bit term_on_no_change = 0,
                                                xip_uint max_iterations = 0,
                                                xip_uint id = 0);

std::shared_ptr<XIP_LDPC_STAT> make_status_packet(xip_uint code = 0,
                                                  xip_uint op = 0,
                                                  xip_bit crc_type = 0,
                                                  xip_uint z_j = 0,
                                                  xip_uint z_set = 0,
                                                  xip_uint bg = 0,
                                                  xip_uint mb = 0,
                                                  xip_bit hard_op = 0,
                                                  xip_bit pass = 0,
                                                  xip_bit term_pass = 0,
                                                  xip_bit term_no_change = 0,
                                                  xip_uint dec_iter = 0,
                                                  xip_uint id = 0);

} // namespace sdfec_cmodel
