/**
 * @file dlms_wrapper.h
 * @brief DLMS TCP/IP Wrapper Profile (IEC 62056-47)
 */

#ifndef DLMS_WRAPPER_H
#define DLMS_WRAPPER_H

#include "dlms_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DLMS_WRAPPER_HEADER_LEN     8
#define DLMS_WRAPPER_VERSION        0x0001

typedef struct {
    uint16_t version;
    uint16_t source_wport;      /**< Client SAP */
    uint16_t destination_wport; /**< Server SAP */
    uint16_t length;            /**< Payload APDU length */
    const uint8_t *apdu;        /**< Pointer to xDLMS APDU */
} dlms_wrapper_packet_t;

/** Parse incoming wrapper packet */
bool dlms_wrapper_parse(const uint8_t *buf, uint16_t len, dlms_wrapper_packet_t *pkt);

/** Build wrapper header for response APDU */
uint16_t dlms_wrapper_build_response(
    uint16_t server_sap,
    uint16_t client_sap,
    const uint8_t *apdu,
    uint16_t apdu_len,
    uint8_t *out_buf,
    uint16_t max_out_len);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_WRAPPER_H */
