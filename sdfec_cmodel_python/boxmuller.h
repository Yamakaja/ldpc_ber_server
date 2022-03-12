#pragma once

#include <cstdint>
#include <cstdlib>
#include <tuple>

namespace sdfec_cmodel {

template <unsigned int I, unsigned int F>
struct fxpnt {
    static_assert(I + F < 63, "Width of fixed point value must be < 63!");

    int64_t m_val;

    static constexpr int64_t VALUE_MASK = (1L << (I + F)) - 1;
    static constexpr int64_t FRACTION_MASK = (1L << F) - 1;
    static constexpr int64_t INTEGER_MASK = ((1L << I) - 1) << F;
    static constexpr int64_t INTEGER_BASE = 1L << F;
    static constexpr int64_t FRACTION_LENGTH = F;
    static constexpr int64_t INTEGER_LENGTH = I;
    static constexpr int64_t SIGN_MASK = ~VALUE_MASK;
    static constexpr int64_t MSB = 1L << (I + F - 1);

    static int64_t sign_extend(int64_t v)
    {
        v = VALUE_MASK & v;

        if (v & MSB)
            v |= SIGN_MASK;

        return v;
    }

    explicit fxpnt() : m_val(0L) {}

    explicit fxpnt(int64_t v) : m_val(sign_extend(v)) {}

    explicit fxpnt(int64_t i, int64_t f) : m_val(((INTEGER_MASK & i) << F) | (FRACTION_MASK & f)) {}

    explicit operator int64_t() const { return m_val; }

    explicit operator double() const
    {
        int64_t val = m_val & VALUE_MASK;
        if (val & MSB)
            val |= SIGN_MASK;

        return static_cast<double>(m_val) / INTEGER_BASE;
    }

    fxpnt<I, F> operator+(fxpnt<I, F> b) const { return fxpnt<I, F>(m_val + b.m_val); }

    fxpnt<I, F> operator+(int64_t b) const { return fxpnt<I, F>(m_val + (b << F)); }

    fxpnt<I, F> operator-(fxpnt<I, F> b) const { return fxpnt<I, F>(m_val - b.m_val); }

    fxpnt<I, F> operator-(int64_t b) const { return fxpnt<I, F>(m_val - (b << F)); }

    template <unsigned int NI, unsigned int NF>
    explicit operator fxpnt<NI, NF>() const
    {
        int fdiff = NF - F;

        if (fdiff > 0)
            return { m_val << fdiff };
        else
            return { m_val >> -fdiff };
    };

    template <unsigned int NI, unsigned int NF>
    fxpnt<NI, NF> truncate() const
    {
        static_assert(NF <= F && NI <= I, "truncate() must reduce fraction width!");
        unsigned int fdiff = F - NF;

        return fxpnt<NI, NF>(m_val >> fdiff);
    }

    template <unsigned int NI, unsigned int NF>
    fxpnt<NI, NF> round() const
    {
        static_assert(NF < F, "round() must reduce fraction width!");
        unsigned int fdiff = F - NF;

        return fxpnt<NI, NF>((m_val + (1L << (fdiff - 1))) >> fdiff);
    }

    template <unsigned int NI, unsigned int NF>
    fxpnt<NI, NF> extend() const
    {
        static_assert(NF >= F && NI >= I,
                      "Either N or I should be extended, and both must be larger or equal to the current value!");
        unsigned int fdiff = NF - F;

        return fxpnt<NI, NF>(m_val << fdiff);
    }

    template <unsigned int BI, unsigned int BF>
    fxpnt<BI + I, BF + F> operator*(fxpnt<BI, BF> b) const
    {
        return fxpnt<BI + I, BF + F>(m_val * b.m_val);
    }

    template <unsigned int NI, unsigned int NF>
    fxpnt<NI, NF> cast()
    {
        return fxpnt<NI, NF>(m_val);
    }

    fxpnt<I, F> operator*(int64_t b) const { return fxpnt<I, F>(m_val * b); }

    fxpnt<I, F> operator<<(int i) const { return fxpnt<I, F>(i >= 0 ? (m_val << i) : (m_val >> -i)); }

    fxpnt<I, F> operator>>(int i) const { return fxpnt<I, F>(i >= 0 ? (m_val >> i) : (m_val << -i)); }

    fxpnt<I, F> operator~() const { return fxpnt<I, F>(~m_val); }

    fxpnt<I, F> operator-() const { return fxpnt<I, F>(-m_val); }

    int lzd() const
    {
        static_assert(I + F < 64);
        int count = 0;
        for (int64_t v = MSB; v && !(v & m_val); v >>= 1)
            count++;

        return count;
    }
};

extern int64_t fxpnt_log_rom[256][3];
extern int64_t fxpnt_sqrt_rom[128][2];
extern int64_t fxpnt_trig_rom[128][4];

fxpnt<1, 27> fxpnt_pp_log(fxpnt<1, 31> x);

fxpnt<2, 16> fxpnt_pp_sqrt(fxpnt<1, 19> x, bool odd);

std::tuple<fxpnt<2, 16>, fxpnt<2, 16>> fxpnt_pp_trig(fxpnt<1, 14> x);

std::tuple<fxpnt<5, 11>, fxpnt<5, 11>> bitexact_boxmuller(int64_t u0, int64_t u1, int64_t u2);

std::tuple<double, double> bitexact_gaussian(int64_t u0, int64_t u1, int64_t u2);

std::tuple<double, double> test_boxmuller();

} // namespace sdfec_cmodel
