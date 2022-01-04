#pragma once

#include <vector>
#include <memory>
#include <string>

extern "C" {
#include "sdfec_usr_intf.h"
}

class ldpc_code {
private:
    // Source of truth
    xsdfec_user_ldpc_code_params m_ldpc_code_params{};
    xsdfec_user_ldpc_table_offsets m_ldpc_table_offsets{};

    // Structure required by driver
    xsdfec_ldpc_params m_ldpc_params{};

    std::vector<uint32_t> m_sc_table{};
    std::vector<uint32_t> m_la_table{};
    std::vector<uint32_t> m_qc_table{};

    bool m_dec_ok{};
    bool m_enc_ok{};

public:
    ldpc_code();

    ldpc_code& operator=(ldpc_code&) = delete;
    ldpc_code(const ldpc_code&) = delete;

    virtual ~ldpc_code();

    std::vector<uint32_t>& get_sc_table() { return m_sc_table; }
    std::vector<uint32_t>& get_la_table() { return m_la_table; }
    std::vector<uint32_t>& get_qc_table() { return m_qc_table; }

    void set_sc_table(const std::vector<uint32_t>& v) { m_sc_table = v; }
    void set_la_table(const std::vector<uint32_t>& v) { m_la_table = v; }
    void set_qc_table(const std::vector<uint32_t>& v) { m_qc_table = v; }

    uint32_t get_n() const { return m_ldpc_code_params.n; }
    void set_n(uint32_t v) { m_ldpc_code_params.n = v; }

    uint32_t get_k() const { return m_ldpc_code_params.k; }
    void set_k(uint32_t v) { m_ldpc_code_params.k = v; }

    uint32_t get_psize() const { return m_ldpc_code_params.psize; }
    void set_psize(uint32_t v) { m_ldpc_code_params.psize = v; }

    uint32_t get_nlayers() const { return m_la_table.size(); }
    void set_nlayers(uint32_t v) { m_la_table.resize(v); }

    uint32_t get_nqc() const { return m_qc_table.size(); }
    void set_nqc(uint32_t v) { m_qc_table.resize(v); }

    uint32_t get_nmqc() const { return m_ldpc_code_params.nmqc; }
    void set_nmqc(uint32_t v) { m_ldpc_code_params.nmqc = v; }

    uint32_t get_nm() const { return m_ldpc_code_params.nm; }
    void set_nm(uint32_t v) { m_ldpc_code_params.nm = v; }

    uint32_t get_norm_type() const { return m_ldpc_code_params.norm_type; }
    void set_norm_type(uint32_t v) { m_ldpc_code_params.norm_type = v; }

    uint32_t get_no_packing() const { return m_ldpc_code_params.no_packing; }
    void set_no_packing(uint32_t v) { m_ldpc_code_params.no_packing = v; }

    uint32_t get_special_qc() const { return m_ldpc_code_params.special_qc; }
    void set_special_qc(uint32_t v) { m_ldpc_code_params.special_qc = v; }

    uint32_t get_no_final_parity() const { return m_ldpc_code_params.no_final_parity; }
    void set_no_final_parity(uint32_t v) { m_ldpc_code_params.no_final_parity = v; }

    uint32_t get_max_schedule() const { return m_ldpc_code_params.max_schedule; }
    void set_max_schedule(uint32_t v) { m_ldpc_code_params.max_schedule = v; }

    bool get_dec_ok() const { return m_dec_ok; }
    void set_dec_ok(bool v) { m_dec_ok = v; }

    bool get_enc_ok() const { return m_enc_ok; }
    void set_enc_ok(bool v) { m_enc_ok = v; }

    void prepare();

    xsdfec_ldpc_params& get_ldpc_params() { return m_ldpc_params; }

    std::string to_json();

    std::string get_hash();
};

class sdfec {
private:
    const int       m_id{};
    int             m_fd{};
    std::string     m_dev_name{};
    bool            m_in_order{};
    bool            m_bypass{};

public:
    sdfec(int id, bool in_order);

    sdfec(const sdfec&) = delete;
    sdfec& operator=(sdfec&) = delete;

    virtual ~sdfec();

    int get_id() const { return m_id; }

    int get_fd() const { return m_fd; }

    bool is_in_order() { return m_in_order; }

    void set_ldpc_code(ldpc_code& code);

    void start();
    
    void stop();

    xsdfec_state get_state();

    bool is_active();

    bool is_bypass() { return m_bypass; }
    void set_bypass(bool v);

    void clear_stats();

    std::shared_ptr<xsdfec_stats> get_stats();

    std::string to_json();
};
