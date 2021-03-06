#include "xoroshiro128plus.h"

#include <stdint.h>
#include <stdlib.h>

static inline uint64_t rotl(const uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }

void xoroshiro128plus_jump(xoroshiro128plus_t* state)
{
    static const uint64_t JUMP[] = { 0xdf900294d8f554a5, 0x170865df4b3201fc };

    uint64_t s0 = 0;
    uint64_t s1 = 0;
    for (uint64_t i = 0; i < sizeof JUMP / sizeof *JUMP; i++)
        for (int b = 0; b < 64; b++) {
            if (JUMP[i] & UINT64_C(1) << b) {
                s0 ^= state->s[0];
                s1 ^= state->s[1];
            }

            xoroshiro128plus_next(state);
        }

    state->s[0] = s0;
    state->s[1] = s1;
}

static void xoroshiro128plus_polyjump_pow2(xoroshiro128plus_t* state, size_t i)
{
    const uint64_t *JUMP = xoro_jump_polynomials[i];

    uint64_t s0 = 0;
    uint64_t s1 = 0;
    for (uint64_t i = 0; i < 2; i++)
        for (int b = 0; b < 64; b++) {
            if (JUMP[i] & UINT64_C(1) << b) {
                s0 ^= state->s[0];
                s1 ^= state->s[1];
            }
            xoroshiro128plus_next(state);
        }

    state->s[0] = s0;
    state->s[1] = s1;
}

void xoroshiro128plus_forward(xoroshiro128plus_t *state, uint64_t amount)
{
    if (amount & 1)
        xoroshiro128plus_next(state);

    amount >>= 1;

    for (size_t i = 1; i < 64 && amount; i++) {
        if (amount & 1)
            xoroshiro128plus_polyjump_pow2(state, i);

        amount >>= 1;
    }
}

uint64_t xoroshiro128plus_next(xoroshiro128plus_t* state)
{
    const uint64_t s0 = state->s[0];
    uint64_t s1 = state->s[1];
    const uint64_t result = s0 + s1;

    s1 ^= s0;
    state->s[0] = rotl(s0, 24) ^ s1 ^ (s1 << 16); // a, b
    state->s[1] = rotl(s1, 37);                   // c

    return result;
}

void xoroshiro128plus_init(xoroshiro128plus_t* state, uint64_t seed)
{
    state->x = seed;
    state->s[0] = splitmix64_next(state);
    state->s[1] = splitmix64_next(state);
}

uint64_t splitmix64_next(xoroshiro128plus_t* state)
{
    uint64_t z = (state->x += 0x9e3779b97f4a7c15);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    return z ^ (z >> 31);
}
