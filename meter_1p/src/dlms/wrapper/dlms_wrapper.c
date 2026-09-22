/**
 * @file dlms_wrapper.c
 * @brief DLMS TCP/IP Wrapper Profile Implementation (IEC 62056-47)
 */

#include "dlms_wrapper.h"
#include <string.h>

bool dlms_wrapper_parse(const uint8_t *buf, uint16_t len, dlms_wrapper_packet_t *pkt) {
    if (!buf || len < DLMS_WRAPPER_HEADER_LEN || !pkt) return false;

    pkt->version = ((uint16_t)buf[0] << 8) | buf[1];
    pkt->source_wport = ((uint16_t)buf[2] << 8) | buf[3];
    pkt->destination_wport = ((uint16_t)buf[4] << 8) | buf[5];
    pkt->length = ((uint16_t)buf[6] << 8) | buf[7];

    if (pkt->version != DLMS_WRAPPER_VERSION) return false;
    if (len < DLMS_WRAPPER_HEADER_LEN + pkt->length) return false;

    pkt->apdu = buf + DLMS_WRAPPER_HEADER_LEN;
    return true;
}

uint16_t dlms_wrapper_build_response(
    uint16_t server_sap,
    uint16_t client_sap,
    const uint8_t *apdu,
    uint16_t apdu_len,
    uint8_t *out_buf,
    uint16_t max_out_len)
{
    uint16_t total = DLMS_WRAPPER_HEADER_LEN + apdu_len;
    if (total > max_out_len || !out_buf || !apdu) return 0;

    /* Version: 0x0001 */
    out_buf[0] = (uint8_t)(DLMS_WRAPPER_VERSION >> 8);
    out_buf[1] = (uint8_t)(DLMS_WRAPPER_VERSION & 0xFF);

    /* Source wPort: Server SAP */
    out_buf[2] = (uint8_t)(server_sap >> 8);
    out_buf[3] = (uint8_t)(server_sap & 0xFF);

    /* Destination wPort: Client SAP */
    out_buf[4] = (uint8_t)(client_sap >> 8);
    out_buf[5] = (uint8_t)(client_sap & 0xFF);

    /* Length */
    out_buf[6] = (uint8_t)(apdu_len >> 8);
    out_buf[7] = (uint8_t)(apdu_len & 0xFF);

    memcpy(out_buf + DLMS_WRAPPER_HEADER_LEN, apdu, apdu_len);
    return total;
}
