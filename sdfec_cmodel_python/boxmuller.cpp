#include "boxmuller.h"

#include <fmt/core.h>

#include <iomanip>
#include <iostream>

#define DEBUG_CND 0
#define DEBUG_CND_BM DEBUG_CND
#define DEBUG_CND_LOG DEBUG_CND
#define DEBUG_CND_SQRT DEBUG_CND
#define DEBUG_CND_TRIG DEBUG_CND

#define STR(x) #x

#if DEBUG_CND_LOG
#define DEBUG_LOG(v) std::cout << std::hex << "LOG: " << STR(v) << " = 0x" << v.m_val << std::endl;
#else
#define DEBUG_LOG(v)
#endif

#if DEBUG_CND_BM
#define DEBUG_BM(v) std::cout << std::hex << "BM: " << STR(v) << " = 0x" << v.m_val << std::endl;
#else
#define DEBUG_BM(v)
#endif

#if DEBUG_CND_SQRT
#define DEBUG_SQRT(v) std::cout << std::hex << "SQRT: " << STR(v) << " = 0x" << v.m_val << std::endl;
#else
#define DEBUG_SQRT(v)
#endif

#if DEBUG_CND_TRIG
#define DEBUG_TRIG(v) std::cout << std::hex << "TRIG: " << STR(v) << " = 0x" << v.m_val << std::endl;
#else
#define DEBUG_TRIG(v)
#endif

namespace sdfec_cmodel {

fxpnt<1, 27> fxpnt_pp_log(fxpnt<1, 31> x)
{
    static const size_t FRAC_LENGTH = 23;
    int64_t x_B = x.m_val >> FRAC_LENGTH;
    fxpnt<1, 22> x_A{ (x.m_val & ((1L << FRAC_LENGTH) - 1)) >> 1 };
    DEBUG_LOG(x_A);

    int64_t* coeff = fxpnt_log_rom[x_B];
    // static size_t COEFF_WIDTHS[3] = {14, 23, 31};

    if (DEBUG_CND_LOG)
        std::cout << std::hex << "LOG: coeffs[3] = { " << coeff[0] << ", " << coeff[1] << ", " << coeff[2] << " };"
                  << std::endl;

    fxpnt<1, 27> r_1_y = x_A.truncate<1, 13>() * fxpnt<0, 14>(coeff[0]);
    DEBUG_LOG(r_1_y);

    fxpnt<10, 14> coeff_1{ coeff[1] };

    fxpnt<10, 27> r_2_y = r_1_y.extend<10, 27>() + coeff_1.extend<10, 27>();
    DEBUG_LOG(r_2_y);

    fxpnt<9, 14> w_3_a = r_2_y.truncate<9, 14>();
    DEBUG_LOG(w_3_a);

    fxpnt<10, 14> w_3_y = (w_3_a * x_A).truncate<10, 14>();
    DEBUG_LOG(w_3_y);

    fxpnt<17, 14> r_o = w_3_y.extend<17, 14>() + fxpnt<17, 14>(coeff[2]);
    DEBUG_LOG(r_o);

    fxpnt<17, 11> y_e = r_o.truncate<17, 11>();
    DEBUG_LOG(y_e);

    return fxpnt<1, 27>(y_e.m_val);
}

fxpnt<2, 16> fxpnt_pp_sqrt(fxpnt<1, 19> x, bool odd)
{
    static const size_t FRAC_LENGTH = 13;

    int64_t x_B = x.m_val >> FRAC_LENGTH;
    if (DEBUG_CND_SQRT)
        std::cout << "SQRT: "
                  << "x_B = " << x_B << std::endl;
    fxpnt<1, 13> x_A{ x.m_val & ((1 << FRAC_LENGTH) - 1) };
    DEBUG_SQRT(x_A);

    int64_t* coeff = fxpnt_sqrt_rom[x_B + odd * 64];
    if (DEBUG_CND_SQRT)
        std::cout << std::hex << "SQRT: coeffs[2] = { " << coeff[0] << ", " << coeff[1] << " };" << std::endl;

    fxpnt<6, 26> r_1_y = x_A.extend<6, 13>() * fxpnt<0, 13>(coeff[0]);
    DEBUG_SQRT(r_1_y);

    fxpnt<6, 26> r_2_y = r_1_y + fxpnt<6, 26>(coeff[1] << 13);
    DEBUG_SQRT(r_2_y);

    fxpnt<2, 16> r_o = r_2_y.truncate<6, 11>().cast<2, 16>() + 1L;
    DEBUG_SQRT(r_o);

    return r_o;
}

std::tuple<fxpnt<2, 16>, fxpnt<2, 16>> fxpnt_pp_trig(fxpnt<1, 14> x)
{
    static const size_t FRAC_LENGTH = 7;

    int64_t x_B = x.m_val >> FRAC_LENGTH;
    if (x_B < 0 || x_B > 127)
        throw std::runtime_error(fmt::format("Encountered invalid x_B in fxpnt_pp_trig: {}", x_B));

    fxpnt<1, 7> x_A{ x.m_val & ((1 << FRAC_LENGTH) - 1) };
    DEBUG_TRIG(x_A);

    int64_t* coeff = fxpnt_trig_rom[x_B];
    if (DEBUG_CND_TRIG)
        std::cout << std::hex << "TRIG: coeffs[4] = { " << coeff[0] << ", " << coeff[1] << ", " << coeff[2] << ", "
                  << coeff[3] << " };" << std::endl;

    fxpnt<1, 11> c_1_sin{ coeff[0] };
    fxpnt<1, 11> c_1_cos{ coeff[2] };

    DEBUG_TRIG(c_1_sin);
    DEBUG_TRIG(c_1_cos);

    fxpnt<2, 18> r_1_y_sin = c_1_sin * x_A;
    fxpnt<2, 18> r_1_y_cos = c_1_cos * x_A;

    DEBUG_TRIG(r_1_y_sin);
    DEBUG_TRIG(r_1_y_cos);

    fxpnt<2, 24> r_2_y_sin = r_1_y_sin.cast<2, 24>() + fxpnt<2, 24>(coeff[1] << 7);
    fxpnt<2, 24> r_2_y_cos = r_1_y_cos.cast<2, 24>() + fxpnt<2, 24>(coeff[3] << 7);

    DEBUG_TRIG(r_2_y_sin);
    DEBUG_TRIG(r_2_y_cos);

    fxpnt<2, 16> ret_sin = r_2_y_sin.truncate<2, 16>();
    fxpnt<2, 16> ret_cos = r_2_y_cos.truncate<2, 16>();

    DEBUG_TRIG(ret_sin);
    DEBUG_TRIG(ret_cos);

    return { ret_sin, ret_cos };
}

std::tuple<fxpnt<5, 11>, fxpnt<5, 11>> bitexact_boxmuller(int64_t r_u0, int64_t r_u1, int64_t r_u2)
{

    fxpnt<48, 0> u0{ r_u0 };
    fxpnt<1, 31> u2{ 0, r_u2 };

    /* =========== LOG ========== */
    fxpnt<7, 0> r_e_exp{ static_cast<int>(u0.lzd() + 1) };
    DEBUG_BM(r_e_exp);

    fxpnt<1, 27> y_e = fxpnt_pp_log(u2);
    fxpnt<9, 26> r_e_y = y_e.truncate<1, 26>().extend<9, 26>();
    DEBUG_BM(r_e_y);

    static const fxpnt<1, 26> LN2{ 46516319L };
    DEBUG_BM(LN2);

    fxpnt<8, 26> r_e_exp_ln = r_e_exp * LN2;
    DEBUG_BM(r_e_exp_ln);

    fxpnt<9, 26> r_e_int = r_e_exp_ln.extend<9, 26>() - r_e_y;
    DEBUG_BM(r_e_int);

    fxpnt<6, 25> r_e = r_e_int.truncate<6, 25>();
    DEBUG_BM(r_e);

    /* =========== SQRT ========== */
    fxpnt<5, 0> w_f_p{ r_e.lzd() };
    DEBUG_BM(w_f_p);

    fxpnt<6, 0> r_f_exp = fxpnt<6, 0>((w_f_p.m_val & 0x1fL) - 6L);
    DEBUG_BM(r_f_exp);

    fxpnt<6, 0> r_f_exp_d = -r_f_exp;
    DEBUG_BM(r_f_exp_d);

    if (r_f_exp_d.m_val & 1L)
        r_f_exp_d = r_f_exp_d - 1L;

    DEBUG_BM(r_f_exp_d);

    /*
    int64_t r_e_exp_d = -r_e_exp.m_val;
    if (r_e_exp_d & 1)
        r_e_exp_d -= 1;

    if (DEBUG_CND_BM)
        std::cout << std::hex << "BM: r_e_exp_d = 0x" << r_e_exp_d << std::endl;
        */

    fxpnt<6, 25> w_f_x = r_e << static_cast<int>(r_f_exp.m_val);
    DEBUG_BM(w_f_x);

    fxpnt<1, 19> r_f_x = fxpnt<1, 19>(0x7FFFFL & (w_f_x.m_val >> 5));
    DEBUG_BM(r_f_x);

    fxpnt<2, 16> w_f_y = fxpnt_pp_sqrt(r_f_x, r_f_exp.m_val & 1L);
    DEBUG_BM(w_f_y);

    fxpnt<5, 16> r_f_y = w_f_y.extend<5, 16>();
    DEBUG_BM(r_f_y);

    fxpnt<5, 16> w_f = r_f_y << static_cast<int>(r_f_exp_d.m_val >> 1);
    DEBUG_BM(w_f);

    fxpnt<7, 11> r_f = w_f.truncate<5, 13>().cast<7, 11>();
    DEBUG_BM(r_f);

    /* =========== SIN/COS ========== */
    int quadrant = 0x3 & (r_u1 >> 14);

    auto sincos = fxpnt_pp_trig(fxpnt<1, 14>(r_u1 & 0x3fFFL));
    fxpnt<2, 16> y_sin = std::get<0>(sincos);
    fxpnt<2, 16> y_cos = std::get<1>(sincos);

    fxpnt<2, 16> r_t_g_0;
    fxpnt<2, 16> r_t_g_1;

    switch (quadrant) {
    case 0:
        r_t_g_0 = y_sin;
        r_t_g_1 = y_cos;
        break;
    case 1:
        r_t_g_0 = y_cos;
        r_t_g_1 = -y_sin;
        break;
    case 2:
        r_t_g_0 = -y_sin;
        r_t_g_1 = -y_cos;
        break;
    case 3:
        r_t_g_0 = -y_cos;
        r_t_g_1 = y_sin;
        break;
    }

    DEBUG_BM(r_t_g_0);
    DEBUG_BM(r_t_g_1);

    fxpnt<9, 27> r_o_0 = r_f * r_t_g_0;
    fxpnt<9, 27> r_o_1 = r_f * r_t_g_1;

    DEBUG_BM(r_o_0);
    DEBUG_BM(r_o_1);

    fxpnt<5, 11> r_x_0 = r_o_0.truncate<7, 9>().cast<5, 11>();
    fxpnt<5, 11> r_x_1 = r_o_1.truncate<7, 9>().cast<5, 11>();

    DEBUG_BM(r_x_0);
    DEBUG_BM(r_x_1);

    return { r_x_0, r_x_1 };
}

std::tuple<double, double> bitexact_gaussian(int64_t u0, int64_t u1, int64_t u2)
{
    auto ret = bitexact_boxmuller(u0, u1, u2);
    return { (double)std::get<0>(ret), (double)std::get<1>(ret) };
}

std::tuple<double, double> test_boxmuller() { return bitexact_gaussian(0x688cf16be206, 0x1730, 0x77f1ec5a); }

}; // namespace sdfec_cmodel
