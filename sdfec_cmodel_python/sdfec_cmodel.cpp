#include "sdfec_cmodel.h"

#include <fmt/core.h>

#include <string.h>
#include <memory>
#include <vector>

namespace {
struct la_table_contents {
    uint32_t a;
    uint32_t b;
    uint32_t c;
};

struct qc_table_contents {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
};
}; // namespace


namespace sdfec_cmodel {

ldpc_parameter_wrapper::~ldpc_parameter_wrapper() { xip_sd_fec_v1_1_destroy_ldpc_params(&params, nullptr, nullptr); }

std::shared_ptr<ldpc_parameter_wrapper> gen_ldpc_params(char* src_file)
{
    auto ldpc_params = std::make_shared<ldpc_parameter_wrapper>();
    xip_uint status = xip_sd_fec_v1_1_gen_ldpc_params(src_file, &ldpc_params->params, nullptr, nullptr);

    ldpc_params->enc_OK = status & XIP_SD_FEC_v1_1_LDPC_CODE_ENC_ONLY;
    ldpc_params->dec_OK = status & XIP_SD_FEC_v1_1_LDPC_CODE_DEC_ONLY;
    return ldpc_params;
}

std::vector<uint32_t> get_sc_table(const std::shared_ptr<ldpc_parameter_wrapper> self)
{
    size_t len = (self->params.nlayers >> 2) + !!(self->params.nlayers & 0x3);
    std::vector<uint32_t> vec(len);

    if (len == 0)
        return {};

    for (uint32_t i = 0; i < len; i++) {
        uint32_t value = 0;

        uint32_t max = 4;
        if (i + 1 == len && self->params.nlayers & 0x3)
            max = self->params.nlayers & 0x3;

        for (uint32_t j = 0; j < max; j++)
            value |= (self->params.sc_table[i * 4 + j] & 0xF) << (j * 4);

        vec[i] = value;
    }

    return vec;
}

std::vector<uint32_t> get_la_table(const std::shared_ptr<ldpc_parameter_wrapper> self)
{
    la_table_contents* contents = reinterpret_cast<la_table_contents*>(self->params.la_table);

    std::vector<uint32_t> vec(self->params.nlayers);
    for (uint32_t i = 0; i < self->params.nlayers; i++) {
        auto& v = contents[i];

        vec[i] = v.a + (v.b << 8);
    }

    return vec;
}

std::vector<uint32_t> get_qc_table(const std::shared_ptr<ldpc_parameter_wrapper> self)
{

    qc_table_contents* contents = reinterpret_cast<qc_table_contents*>(self->params.qc_table);

    std::vector<uint32_t> vec(self->params.nqc);

    for (uint32_t i = 0; i < self->params.nqc; i++) {
        auto& v = contents[i];

        int v3 = 0;

        // Third value. Mapping not obvious ...
        switch (v.c) {
        case 0x00010000:
            v3 = 0;
            break;
        case 0x00010001:
            v3 = 2;
            break;
        case 0x00010100:
            v3 = 4;
            break;
        case 0x00010101:
            v3 = 6;
            break;
        default:
            throw std::runtime_error(fmt::format("Encountered unhandled value 0x{:08x} in "
                                                 "third column of qc table output.",
                                                 v.c));
        }

        vec[i] = v.a + (v.b << 8) + (v3 << 16);
    }

    return vec;
}

sdfec_core::sdfec_core(std::string name,
                       std::string code_file,
                       fec_mode mode,
                       ldpc_standard standard,
                       int scale_override,
                       bool bypass,
                       quant_mode quantization_mode)
    : m_name(name), m_code_file(code_file), m_version(xip_sd_fec_v1_1_get_version())
{
    m_config.name = m_name.c_str();
    m_config.fec = static_cast<xip_uint>(mode);
    m_config.standard = static_cast<xip_uint>(standard);

    if (scale_override < 0)
        scale_override = 16;
    else if (scale_override > 15)
        scale_override = 16;

    m_config.scale_override = static_cast<xip_uint>(scale_override);

    m_config.ldpc_code_initialization = m_code_file.c_str();
    m_config.bypass = static_cast<xip_bit>(bypass);
    m_config.ip_quant_mode = static_cast<xip_uint>(quantization_mode);

    m_fec_handle = xip_sd_fec_v1_1_create(&m_config, nullptr, nullptr);
    if (!m_fec_handle)
        throw std::runtime_error("xip_sd_fec_v1_1_create(): Failed, returned nullptr.");
}

sdfec_core::~sdfec_core() { xip_sd_fec_v1_1_destroy(m_fec_handle); }

void sdfec_core::reset() { xip_sd_fec_v1_1_reset(m_fec_handle); }

void sdfec_core::add_ldpc_code(int number, std::shared_ptr<ldpc_parameter_wrapper> code)
{
    if (number < 1 || number > 32)
        throw std::runtime_error(fmt::format("Code-id out of range: {}", number));

    xip_status status = xip_sd_fec_v1_1_add_ldpc_params(m_fec_handle, number, &code->params);

    if (status == XIP_STATUS_ERROR)
        throw std::runtime_error("xip_sd_fec_v1_1_add_ldpc_params: Failed");
}

void sdfec_core::set_turbo_params(uint32_t alg, uint32_t scale)
{
    XIP_TURBO_DEC_PARAM params = { alg, scale };

    xip_status ret = xip_sd_fec_v1_1_set_turbo_params(m_fec_handle, &params);

    if (ret == XIP_STATUS_ERROR)
        throw std::runtime_error("Failed to update turbo parameters!");
}

int sdfec_core::get_num_loaded_ldpc_codes() { return xip_sd_fec_v1_1_get_num_ldpc_codes(m_fec_handle); }

std::tuple<std::shared_ptr<xip_array_bit>, std::shared_ptr<xip_array_real>, std::shared_ptr<XIP_LDPC_STAT>>
sdfec_core::process(std::shared_ptr<XIP_LDPC_CTRL> ctrl, std::shared_ptr<xip_array_real> data) {
    auto sd_out = std::shared_ptr<xip_array_real>(xip_array_real_create(), xip_array_deleter_real());
    auto hd_out = std::shared_ptr<xip_array_bit>(xip_array_bit_create(), xip_array_deleter_bit());
    auto status = std::make_shared<XIP_LDPC_STAT>();

    xip_status ret = xip_sd_fec_v1_1_process(m_fec_handle,
                                             ctrl.get(),
                                             data.get(),
                                             hd_out.get(),
                                             sd_out.get(),
                                             status.get());

    if (ret == XIP_STATUS_ERROR)
        throw std::runtime_error("Decoder failed to handle request, check your configuration!");

    return {hd_out, sd_out, status};
}

std::string ctrl_packet_to_string(const std::shared_ptr<XIP_LDPC_CTRL> self)
{
    return fmt::format("ctrl_packet(code = {}, op = {}, z_j = {}, z_set = {}, bg = {}, sc_idx = {}, mb = {}, hard_op = {}, "
                       "include_parity_op = {}, "
                       "crc_type = {}, term_on_pass = {}, term_on_no_change = {}, max_iterations = {})",
                       self->code,
                       self->op,
                       self->z_j,
                       self->z_set,
                       self->bg,
                       self->sc_idx,
                       self->mb,
                       self->hard_op,
                       self->include_parity_op,
                       self->crc_type,
                       self->term_on_pass,
                       self->term_on_no_change,
                       self->max_iterations);
}

std::string status_packet_to_string(const std::shared_ptr<XIP_LDPC_STAT> self)
{
    return fmt::format(
        "status_packet(code = {}, op = {}, crc_type = {}, z_j = {}, z_set = {}, bg = {}, mb = {}, hard_op = {}, pass = {}, "
        "term_pass = {}, term_no_change = {}, dec_iter = {}, id = {})",
        self->code,
        self->op,
        self->crc_type,
        self->z_j,
        self->z_set,
        self->bg,
        self->mb,
        self->hard_op,
        self->pass,
        self->term_pass,
        self->term_no_change,
        self->dec_iter,
        self->id);
}

std::shared_ptr<XIP_LDPC_CTRL> make_ctrl_packet(xip_uint code,
                                                xip_uint op,
                                                xip_uint z_j,
                                                xip_uint z_set,
                                                xip_uint bg,
                                                xip_uint sc_idx,
                                                xip_uint mb,
                                                xip_bit hard_op,
                                                xip_bit include_parity_op,
                                                xip_bit crc_type,
                                                xip_bit term_on_pass,
                                                xip_bit term_on_no_change,
                                                xip_uint max_iterations,
                                                xip_uint id)
{

    auto v = std::make_shared<XIP_LDPC_CTRL>();
    v->code = code;
    v->op = op;
    v->z_j = z_j;
    v->z_set = z_set;
    v->bg = bg;
    v->sc_idx = sc_idx;
    v->mb = mb;
    v->hard_op = hard_op;
    v->include_parity_op = include_parity_op;
    v->crc_type = crc_type;
    v->term_on_pass = term_on_pass;
    v->term_on_no_change = term_on_no_change;
    v->max_iterations = max_iterations;
    v->id = id;

    return v;
}

std::shared_ptr<XIP_LDPC_STAT> make_status_packet(xip_uint code,
                                                  xip_uint op,
                                                  xip_bit crc_type,
                                                  xip_uint z_j,
                                                  xip_uint z_set,
                                                  xip_uint bg,
                                                  xip_uint mb,
                                                  xip_bit hard_op,
                                                  xip_bit pass,
                                                  xip_bit term_pass,
                                                  xip_bit term_no_change,
                                                  xip_uint dec_iter,
                                                  xip_uint id)
{

    auto v = std::make_shared<XIP_LDPC_STAT>();

    v->code = code;
    v->op = op;
    v->crc_type = crc_type;
    v->z_j = z_j;
    v->z_set = z_set;
    v->bg = bg;
    v->mb = mb;
    v->hard_op = hard_op;
    v->pass = pass;
    v->term_pass = term_pass;
    v->term_no_change = term_no_change;
    v->dec_iter = dec_iter;
    v->id = id;

    return v;
}

} // namespace sdfec_cmodel
