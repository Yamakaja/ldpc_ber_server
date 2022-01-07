#include "sdfec.h"

#include <string>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <boost/format.hpp>

#include <openssl/md5.h>

ldpc_code::ldpc_code() {
    m_ldpc_params.sc_table = 0;
    m_ldpc_params.la_table = 0;
    m_ldpc_params.qc_table = 0;
}

ldpc_code::~ldpc_code() {
    std::free(reinterpret_cast<void *>(m_ldpc_params.sc_table));
    std::free(reinterpret_cast<void *>(m_ldpc_params.la_table));
    std::free(reinterpret_cast<void *>(m_ldpc_params.qc_table));
}

void ldpc_code::prepare() {
    std::free(reinterpret_cast<void *>(m_ldpc_params.sc_table));
    std::free(reinterpret_cast<void *>(m_ldpc_params.la_table));
    std::free(reinterpret_cast<void *>(m_ldpc_params.qc_table));

    m_ldpc_code_params.nlayers = m_la_table.size();
    m_ldpc_code_params.nqc = m_qc_table.size();

    m_ldpc_code_params.sc_table = m_sc_table.data();
    m_ldpc_code_params.la_table = m_la_table.data();
    m_ldpc_code_params.qc_table = m_qc_table.data();

    int ret = prepare_ldpc_code(&m_ldpc_code_params,
                                &m_ldpc_table_offsets,
                                &m_ldpc_params,
                                /* code id */ 0);
    if (ret)
        throw std::runtime_error(boost::str(boost::format("Error: prepare_ldpc_code(): %s")
                    % std::strerror(ret)));
}

namespace {

template<typename T>
std::string data_to_hex(const T *data, size_t len) {
    std::stringstream ss{};

    ss << std::hex;

    for (size_t i = 0; i < len; i++)
        ss << std::setfill('0') << std::setw(2) << (int) data[i];

    return ss.str();
}

template<typename T>
std::string vector_to_string(const std::vector<T>& vec) {
    if (vec.size() == 0)
        return "[]";

    std::stringstream ss{};
    ss << "[";

    for (size_t i = 0; i < vec.size()-1; i++)
        ss << vec[i] << ", ";

    ss << vec.back() << "]";
    return ss.str();
}

}

bool ldpc_code::is_valid() const {
    const xsdfec_user_ldpc_code_params& p = m_ldpc_code_params;
    return m_dec_ok &&
        4 <= p.n && p.n <= 32768 &&
        2 <= p.k && p.k <= 32766 &&
        2 <= p.psize && p.psize <= 512 &&
        p.nmqc > 0 &&
        m_sc_table.size() > 0 &&
        m_la_table.size() > 0 &&
        m_qc_table.size() > 0;
}

std::string ldpc_code::to_json() {
    return boost::str(boost::format("{"
        "\"dec_OK\": %d, " 
        "\"enc_OK\": %d, " 
        "\"n\": %u, "
        "\"k\": %u, "
        "\"p\": %u, "
        "\"nlayers\": %u, "
        "\"nqc\": %u, "
        "\"nmqc\": %u, "
        "\"nm\": %u, "
        "\"norm_type\": %u, "
        "\"no_packing\": %u, "
        "\"special_qc\": %u, "
        "\"no_final_parity\": %u, "
        "\"max_schedule\": %u, "
        "\"sc_table\": %s, "
        "\"la_table\": %s, "
        "\"qc_table\": %s}")
            % m_dec_ok
            % m_enc_ok
            % m_ldpc_code_params.n
            % m_ldpc_code_params.k
            % m_ldpc_code_params.psize
            % m_la_table.size()
            % m_qc_table.size()
            % m_ldpc_code_params.nmqc
            % m_ldpc_code_params.nm
            % m_ldpc_code_params.norm_type
            % m_ldpc_code_params.no_packing
            % m_ldpc_code_params.special_qc
            % m_ldpc_code_params.no_final_parity
            % m_ldpc_code_params.max_schedule
            % vector_to_string<uint32_t>(m_sc_table)
            % vector_to_string<uint32_t>(m_la_table)
            % vector_to_string<uint32_t>(m_qc_table));
}

std::string ldpc_code::get_hash() {
    MD5_CTX ctx;

    MD5_Init(&ctx);

    std::string v = this->to_json();
    MD5_Update(&ctx, v.c_str(), v.size());

    uint8_t digest[16];
    MD5_Final(digest, &ctx);

    return data_to_hex<uint8_t>(digest, 16);
}

sdfec::sdfec(int id, bool in_order) :
        m_id(id),
        m_in_order(in_order) {
    m_dev_name = boost::str(boost::format("/dev/xsdfec%d") % id);
    int ret = open_xsdfec(m_dev_name.c_str());
    if (ret < 0)
        throw std::runtime_error(boost::str(boost::format("Error: open_xsdfec(%d): %s")
                    % id % std::strerror(ret)));

    m_fd = ret;

    ret = stop_xsdfec(m_fd);
    if (ret)
        throw std::runtime_error(boost::str(boost::format("Error: stop_xsdfec(): %s")
                    % std::strerror(ret)));

    ret = set_order_xsdfec(m_fd, in_order ? XSDFEC_MAINTAIN_ORDER : XSDFEC_OUT_OF_ORDER);
    if (ret)
        throw std::runtime_error(boost::str(boost::format("Error: set_order_xsdfec(): %s")
                    % std::strerror(ret)));

    ret = set_default_config_xsdfec(m_fd);
    if (ret)
        throw std::runtime_error(boost::str(boost::format("Error: set_default_config_xsdfec(): %s")
                    % std::strerror(ret)));
}

sdfec::~sdfec() {
    if (m_fd)
        close_xsdfec(m_fd);
}

std::string sdfec::to_json() {
    return R"JSON({"status": "not implemented!"})JSON";
}

void sdfec::set_ldpc_code(ldpc_code& code) {
    if (!code.get_dec_ok())
        throw std::runtime_error("Error: This code cannot be used for decode operations!");

    code.prepare(); 

    int ret = add_ldpc_xsdfec(m_fd, &code.get_ldpc_params());
    if (ret)
        throw std::runtime_error(boost::str(boost::format("Error: add_ldpc_xsdfec(): %s") % ret));
}

void sdfec::start() {
    int ret = start_xsdfec(m_fd);
    if (ret)
        throw std::runtime_error(boost::str(boost::format("Error: start_xsdfec(): %s") % ret));
}

void sdfec::stop() {
    int ret = stop_xsdfec(m_fd);
    if (ret)
        throw std::runtime_error(boost::str(boost::format("Error: stop_xsdfec(): %s") % ret));
}

xsdfec_state sdfec::get_state() {
    xsdfec_status status;
    int ret = get_status_xsdfec(m_fd, &status);

    if (ret)
        throw std::runtime_error(boost::str(boost::format("Error: get_status_xsdfec(): %s") % ret));

    return static_cast<xsdfec_state>(status.state);
}

bool sdfec::is_active() {
    bool active;
    int ret = is_active_xsdfec(m_fd, &active);

    if (ret)
        throw std::runtime_error(boost::str(boost::format("Error: get_status_xsdfec(): %s") % ret));

    return active;
}

void sdfec::set_bypass(bool v) {
    int ret = set_bypass_xsdfec(m_fd, v);
    if (ret)
        throw std::runtime_error(boost::str(boost::format("Error: set_bypass_xsdfec(): %s") % ret));
}

void sdfec::clear_stats() {
    int ret = clear_stats_xsdfec(m_fd);
    if (ret)
        throw std::runtime_error(boost::str(boost::format("Error: clear_stats_xsdfec(): %s") % ret));
}

std::shared_ptr<xsdfec_stats> sdfec::get_stats() {
    std::shared_ptr<xsdfec_stats> stats = std::make_shared<xsdfec_stats>();
    int ret = get_stats_xsdfec(m_fd, stats.get());
    if (ret)
        throw std::runtime_error(boost::str(boost::format("Error: get_stats_xsdfec(): %s") % ret));

    return stats;
}
