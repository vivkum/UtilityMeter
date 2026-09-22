/**
 * @file dlms_crc.h
 * @brief CRC-16-CCITT Calculation for HDLC HCS and FCS (IEC 62056-46)
 */

#ifndef DLMS_CRC_H
#define DLMS_CRC_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DLMS_HDLC_CRC_INIT      0xFFFF
#define DLMS_HDLC_CRC_GOOD      0xF0B8

/** Calculate CRC-16-CCITT over a buffer (accumulative) */
uint16_t dlms_crc16_update(uint16_t crc, const uint8_t *data, size_t len);

/** Finalize CRC-16 (1's complement) */
static inline uint16_t dlms_crc16_finalize(uint16_t crc) {
    return (uint16_t)(crc ^ 0xFFFF);
}

/** Calculate complete CRC-16-CCITT for a buffer */
static inline uint16_t dlms_crc16_calculate(const uint8_t *data, size_t len) {
    return dlms_crc16_finalize(dlms_crc16_update(DLMS_HDLC_CRC_INIT, data, len));
}

#ifdef __cplusplus
}
#endif

#endif /* DLMS_CRC_H */
