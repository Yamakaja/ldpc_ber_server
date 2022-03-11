#pragma once

#include "sdfec_cmodel.h"
#include "xoroshiro128plus.h"

#include <array>
#include <cstdint>
#include <tuple>
#include <vector>

namespace sdfec_cmodel {

class ldpc_ber_tester
{
    static constexpr size_t HISTORY_DEPTH = 35 - 11;
    static constexpr uint64_t BASE_SEED = 0xC3D3EE97;

    static constexpr size_t OFFSET_U_0 = 24 - 11;
    static constexpr size_t OFFSET_U_1 = 12 - 11;
    static constexpr size_t OFFSET_U_2 = 33 - 11;

    size_t m_init_flush{};
    uint64_t m_seed_base{};
    std::shared_ptr<sdfec_core> m_sdfec{};
    std::shared_ptr<XIP_LDPC_PARAMS> m_code{};

    std::array<xoroshiro128plus_t, 12> m_xoros{};
    /*
     * Ring buffer for each instance
     * m_xoro_idx points to the next element that will be overriden
     * and is always incremented % HISTORY_DEPTH
     **/
    std::array<std::array<uint64_t, HISTORY_DEPTH>, 12> m_xoro_history{};
    size_t m_xoro_idx = 0;

    double m_snr = 0;
    double m_snr_scale = 1;
    double m_factor = 0;
    double m_offset = 0;

    double m_factor_q = 0;
    double m_offset_q = 0;

    std::array<double, 16> m_gaussians;

    size_t m_din_beats;

    void update_snr();

    void xoro_step(bool update_gaussians = false);

    void skip_to_block(ssize_t id);

    uint64_t get_past_value(size_t id, size_t offset);

    std::tuple<uint64_t, uint64_t, uint64_t> get_rnd(size_t idx);

    void get_gaussians(std::array<double, 16>& output);

    double quantize_llr(double x);

public:
    ldpc_ber_tester(uint64_t seed_base, std::shared_ptr<sdfec_core> core, uint64_t init_flush = 45);

    virtual ~ldpc_ber_tester();

    void set_snr(double snr)
    {
        m_snr = snr;
        update_snr();
    }

    void set_snr_scale(double snr_scale)
    {
        m_snr_scale = snr_scale;
        update_snr();
    }

    double get_snr() { return m_snr; }

    double get_snr_scale() { return m_snr_scale; }

    double get_factor() { return m_factor; }

    double get_offset() { return m_offset; }

    size_t get_din_beats() { return m_din_beats; }

    std::tuple<std::shared_ptr<xip_array_real>,
               std::shared_ptr<XIP_LDPC_STAT>,
               std::shared_ptr<xip_array_real>,
               std::shared_ptr<xip_array_bit>>
    simulate_block(uint64_t block,
                   bool hard_op = true,
                   unsigned int code_id = 0,
                   unsigned int max_iterations = 32,
                   bool output_parity_bits = false,
                   bool term_on_pass = true,
                   bool term_on_no_change = false);

    std::vector<std::tuple<uint64_t, uint64_t, uint64_t>> get_rnd_vector(uint64_t block, size_t idx);

    std::vector<double> get_gaussian_vector(uint64_t block, bool quantized = false);
};

}; // namespace sdfec_cmodel
