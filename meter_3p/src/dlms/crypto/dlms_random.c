/**
 * @file dlms_random.c
 * @brief Pseudo-Random Number Generator (xoshiro256** / SplitMix64)
 */

#include "dlms_random.h"
#include <string.h>

static uint64_t s[4] = {
    0x8a5cd789635d2dffULL, 0x121fd2155c472f96ULL,
    0x6de117b0ac5ee88cULL, 0x77f213516c1a827fULL
};

static inline uint64_t rotl(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

static uint64_t next_random(void) {
    const uint64_t result = rotl(s[1] * 5, 7) * 9;
    const uint64_t t = s[1] << 17;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];

    s[2] ^= t;
    s[3] = rotl(s[3], 45);

    return result;
}

void dlms_random_seed(uint32_t seed) {
    uint64_t z = (uint64_t)seed + 0x9e3779b97f4a7c15ULL;
    for (int i = 0; i < 4; i++) {
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        s[i] = z ^ (z >> 31);
    }
}

void dlms_random_get_bytes(uint8_t *out, size_t len) {
    while (len >= 8) {
        uint64_t r = next_random();
        memcpy(out, &r, 8);
        out += 8;
        len -= 8;
    }
    if (len > 0) {
        uint64_t r = next_random();
        memcpy(out, &r, len);
    }
}
