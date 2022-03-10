#ifndef H_XOROSHIRO128PLUS
#define H_XOROSHIRO128PLUS

#ifdef __cplusplus
#include <cstdint>
#include <cstdlib>
extern "C" {
#else
#include <stdint.h>
#include <stdlib.h>
#endif

typedef struct xoroshiro128plus_t {
    uint64_t s[2]; // xoroshiro state
    uint64_t x; // Splitmix state
} xoroshiro128plus_t;

extern const uint64_t xoro_base_matrices[64][256];

extern const uint64_t xoro_jump_polynomials[101][2];

uint64_t xoroshiro128plus_next(xoroshiro128plus_t *);

void xoroshiro128plus_forward(xoroshiro128plus_t *, uint64_t steps);

void xoroshiro128plus_jump(xoroshiro128plus_t *);

uint64_t xoroshiro128plus_next(xoroshiro128plus_t *);

uint64_t splitmix64_next(xoroshiro128plus_t *);

void xoroshiro128plus_init(xoroshiro128plus_t *, uint64_t seed);

#ifdef __cplusplus
}
#endif

#endif
