#include "ldpc_ber_tester.h"

#include <fmt/core.h>

#include <algorithm>
#include <cmath>

namespace sdfec_cmodel {

ldpc_ber_tester::ldpc_ber_tester(uint64_t seed_base, std::shared_ptr<sdfec_core> core, uint64_t init_flush)
    : m_init_flush(init_flush), m_seed_base(seed_base), m_sdfec(core)
{
    // Get code information
    m_code = m_sdfec->get_code(0);

    auto n = m_code->n;
    m_din_beats = (n >> 4) + !!(n & 0xf);
}

ldpc_ber_tester::~ldpc_ber_tester() {}

void ldpc_ber_tester::update_snr()
{
    double sigma = std::exp(-std::log(10.0) * m_snr / 20);
    m_offset = -m_snr_scale;
    m_factor = m_snr_scale * sigma;

    m_offset_q = std::round(m_offset * 4) / 4.0;
    m_factor_q = std::round(m_factor * 256) / 256.0;
}

void ldpc_ber_tester::xoro_step(bool update_gaussians)
{
    for (size_t i = 0; i < m_xoros.size(); i++)
        m_xoro_history[i][m_xoro_idx] = xoroshiro128plus_next(&m_xoros[i]);

    if (++m_xoro_idx >= HISTORY_DEPTH)
        m_xoro_idx = 0;

    if (update_gaussians)
        get_gaussians(m_gaussians);
}

uint64_t ldpc_ber_tester::get_past_value(size_t id, uint64_t offset)
{
    if (offset > HISTORY_DEPTH)
        throw std::runtime_error(
            fmt::format("Tried to access history with offset > HISTORY_DEPTH: {} > {}", offset, HISTORY_DEPTH));

    ssize_t idx = static_cast<ssize_t>(m_xoro_idx) - static_cast<ssize_t>(offset);
    if (idx < 0)
        idx += static_cast<ssize_t>(HISTORY_DEPTH);

    return m_xoro_history[id][idx];
}

std::tuple<uint64_t, uint64_t, uint64_t> ldpc_ber_tester::get_rnd(size_t idx)
{
    if (idx > 7)
        throw std::runtime_error(fmt::format("Box-Muller idx out of range: {}", idx));

    if (idx & 1) {
        uint64_t rng_idx_base = 3 * (idx >> 1) + 1;
        uint64_t v_0_0 = get_past_value(rng_idx_base, OFFSET_U_0);
        uint64_t v_0_1 = get_past_value(rng_idx_base + 1, OFFSET_U_0);
        uint64_t u_0 = ((v_0_1 & 0x1FFFFUL) << 31) | (v_0_0 >> 33);

        uint64_t v_1 = get_past_value(rng_idx_base + 1, OFFSET_U_1);
        uint64_t u_1 = 0xFFFFUL & (v_1 >> 17);

        uint64_t v_2 = get_past_value(rng_idx_base + 1, OFFSET_U_2);
        uint64_t u_2 = 0x7FFFFFFFUL & (v_2 >> 33);

        return { u_0, u_1, u_2 };
    } else {
        uint64_t rng_idx_base = 3 * (idx >> 1);
        uint64_t v_0 = get_past_value(rng_idx_base, OFFSET_U_0);
        uint64_t u_0 = ((1UL << 48) - 1) & (v_0 >> 1);

        uint64_t v_1_0 = get_past_value(rng_idx_base, OFFSET_U_1);
        uint64_t v_1_1 = get_past_value(rng_idx_base + 1, OFFSET_U_1);

        uint64_t u_1 = ((v_1_1 & 1) << 15) | (v_1_0 >> 49);

        uint64_t v_2 = get_past_value(rng_idx_base + 1, OFFSET_U_2);
        uint64_t u_2 = (v_2 >> 1) & 0x7fffffffUL;

        return { u_0, u_1, u_2 };
    }
}

void ldpc_ber_tester::skip_to_block(ssize_t id)
{
    m_xoro_idx = 0;

    // Initialize xoroshiros
    for (size_t i = 0; i < m_xoros.size(); i++) {
        xoroshiro128plus_init(&m_xoros[i], BASE_SEED);

        for (size_t j = 0; j < i; j++)
            xoroshiro128plus_jump(&m_xoros[i]);
    }

    for (size_t i = 0; i < 12; i++)
        xoroshiro128plus_forward(&m_xoros[i], m_din_beats * id);

    for (unsigned int i = 0; i < m_init_flush - 1; i++)
        xoro_step();

    xoro_step(true);
}

std::vector<std::tuple<uint64_t, uint64_t, uint64_t>> ldpc_ber_tester::get_rnd_vector(uint64_t block, size_t idx)
{
    std::vector<std::tuple<uint64_t, uint64_t, uint64_t>> vec;

    skip_to_block(block);
    for (uint64_t i = 0; i < m_din_beats; i++) {
        vec.push_back(get_rnd(idx));
        xoro_step();
    }

    return vec;
}

namespace {

int lzd(uint64_t x, int len)
{
    x = x << (64 - len);
    int i = 0;
    for (; i < len; i++)
        if (x & (1UL << 63))
            break;
        else
            x <<= 1;

    return i;
}

void gaussian(uint64_t u_0, uint64_t u_1, uint64_t u_2, double* out)
{
    double exp_e = lzd(u_0, 48) + 1.0;
    double e = 2 * (std::log(2.0) * exp_e - std::log(1.0 + u_2 * 4.656612873077393e-10));

    double f = std::sqrt(e);

    out[0] = std::sin(2 * M_PI * u_1 * 1.52587890625e-05) * f;
    out[1] = std::cos(2 * M_PI * u_1 * 1.52587890625e-05) * f;
}

} // namespace

void ldpc_ber_tester::get_gaussians(std::array<double, 16>& output)
{
    for (int i = 0; i < 8; i++) {
        auto rnd = get_rnd(i);
        gaussian(std::get<0>(rnd), std::get<1>(rnd), std::get<2>(rnd), &output[2 * i]);
    }
}

std::vector<double> ldpc_ber_tester::get_gaussian_vector(uint64_t block, bool quantized)
{
    std::vector<double> values(m_code->n, 0.0);

    skip_to_block(block);

    for (size_t i = 0; i < m_din_beats; i++) {
        for (size_t v = 0; v < 16 && 16 * i + v < m_code->n; v++)
            values[16 * i + v] = quantized ? quantize_llr(m_gaussians[v]) : m_gaussians[v];

        xoro_step(true);
    }

    return values;
}

namespace {
constexpr double POW2(int v) { return static_cast<double>(1 << v); }
} // namespace

double ldpc_ber_tester::quantize_llr(double x)
{
    double q = std::round(x * POW2(11)) / POW2(11); // 5.11

    // 5.11 * 8.8 = 13.19 -> 13.2
    q = std::round((q * m_factor_q) * POW2(2)) / POW2(2);

    // x.2 + x.2
    q += m_offset_q;

    return std::max(std::min(q, 7.5), -7.5);
}

std::tuple<std::shared_ptr<xip_array_real>,
           std::shared_ptr<XIP_LDPC_STAT>,
           std::shared_ptr<xip_array_real>,
           std::shared_ptr<xip_array_bit>>
ldpc_ber_tester::simulate_block(uint64_t block,
                                bool hard_op,
                                unsigned int code_id,
                                unsigned int max_iterations,
                                bool output_parity_bits,
                                bool term_on_pass,
                                bool term_on_no_change)
{
    skip_to_block(block);

    auto ctrl_packet = std::make_shared<XIP_LDPC_CTRL>();
    ctrl_packet->code = code_id;
    ctrl_packet->op = 0;
    ctrl_packet->hard_op = hard_op;
    ctrl_packet->include_parity_op = output_parity_bits;
    ctrl_packet->term_on_pass = term_on_pass;
    ctrl_packet->term_on_no_change = term_on_no_change;
    ctrl_packet->max_iterations = max_iterations;
    ctrl_packet->id = 0;

    auto src_array = std::shared_ptr<xip_array_real>(xip_array_real_create(), xip_array_deleter_real());

    xip_array_real_reserve_data(src_array.get(), m_code->n);
    src_array->data_size = m_code->n;
    xip_array_real_reserve_dim(src_array.get(), 1);
    src_array->dim_size = 1;
    src_array->dim_capacity = 1;
    src_array->dim[0] = m_code->n;

    for (size_t i = 0; i < m_din_beats; i++) {
        for (int j = 0; j < 16 && i * 16 + j < m_code->n; j++)
            src_array->data[16 * i + j] = quantize_llr(m_gaussians[j]);

        xoro_step(true);
    }

    auto ret = m_sdfec->process(ctrl_packet, src_array);

    return { src_array, std::get<2>(ret), std::get<1>(ret), std::get<0>(ret) };
}

}; // namespace sdfec_cmodel
