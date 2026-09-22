/**
 * @file dlms_random.h
 * @brief Pseudo-Random Number Generator for DLMS Challenges (StoC)
 */

#ifndef DLMS_RANDOM_H
#define DLMS_RANDOM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Seed the PRNG with entropy (e.g. metrology timestamp or hardware ADC noise) */
void dlms_random_seed(uint32_t seed);

/** Generate random bytes for HLS challenges */
void dlms_random_get_bytes(uint8_t *out, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_RANDOM_H */
