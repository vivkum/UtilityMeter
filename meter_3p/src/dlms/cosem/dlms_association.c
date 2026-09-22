/**
 * @file dlms_association.c
 * @brief Application Association (AARQ/AARE, RLRQ/RLRE) Implementation
 */

#include "dlms_association.h"
#include "dlms_random.h"
#include "dlms_axdr.h"
#include <string.h>

/* Standard OIDs for DLMS */
static const uint8_t oid_app_ctx_prefix[] = { 0x60, 0x85, 0x74, 0x05, 0x08, 0x01 };
static const uint8_t oid_auth_mech_prefix[] = { 0x60, 0x85, 0x74, 0x05, 0x08, 0x02 };

bool dlms_association_parse_aarq(
    const uint8_t *apdu, uint16_t apdu_len,
    dlms_aarq_params_t *params)
{
    if (!apdu || apdu_len < 4 || !params) return false;
    memset(params, 0, sizeof(dlms_aarq_params_t));
    params->proposed_max_pdu_size = DLMS_MAX_PDU_SIZE;
    params->proposed_conformance = DLMS_DEFAULT_SERVER_CONFORMANCE;

    if (apdu[0] != DLMS_TAG_AARQ) return false;

    dlms_axdr_decoder_t dec;
    dlms_axdr_decoder_init(&dec, apdu, apdu_len);
    dec.offset++; /* Skip 0x60 */

    uint32_t aarq_len;
    if (!dlms_axdr_decode_length(&dec, &aarq_len)) return false;

    while (dec.offset < apdu_len) {
        uint8_t tag = apdu[dec.offset++];
        uint32_t len = 0;
        if (!dlms_axdr_decode_length(&dec, &len)) break;

        if (tag == 0xA1) {
            /* Application Context Name */
            if (dec.offset < apdu_len && apdu[dec.offset] == 0x06) {
                dec.offset++;
                uint32_t oid_len;
                if (dlms_axdr_decode_length(&dec, &oid_len) && oid_len >= 7) {
                    if (memcmp(apdu + dec.offset, oid_app_ctx_prefix, sizeof(oid_app_ctx_prefix)) == 0) {
                        params->app_context = apdu[dec.offset + 6];
                    }
                    dec.offset += oid_len;
                }
            } else {
                dec.offset += len;
            }
        } else if (tag == 0x8A) {
            /* Mechanism Name */
            if (len >= 7 && memcmp(apdu + dec.offset, oid_auth_mech_prefix, sizeof(oid_auth_mech_prefix)) == 0) {
                params->auth_mechanism = apdu[dec.offset + 6];
            }
            dec.offset += len;
        } else if (tag == 0xAC) {
            /* Calling Authentication Value (e.g. password or CtoS challenge) */
            uint16_t end_ac = dec.offset + len;
            if (dec.offset < end_ac && apdu[dec.offset] == 0x80) {
                dec.offset++;
                uint32_t auth_len;
                if (dlms_axdr_decode_length(&dec, &auth_len)) {
                    if (auth_len <= sizeof(params->calling_auth_val)) {
                        memcpy(params->calling_auth_val, apdu + dec.offset, auth_len);
                        params->calling_auth_len = (uint16_t)auth_len;
                    }
                }
            }
            dec.offset = end_ac;
        } else if (tag == 0xBE) {
            /* User Information (xDLMS-Initiate-Request) */
            params->has_user_info = true;
            /* Look inside octet string */
            uint16_t end_be = dec.offset + len;
            if (dec.offset < end_be && apdu[dec.offset] == 0x04) {
                dec.offset++;
                uint32_t octet_len;
                dlms_axdr_decode_length(&dec, &octet_len);
                /* Scan for conformance bitstring (tag 0x5F 0x1F) */
                while (dec.offset + 6 <= end_be) {
                    if (apdu[dec.offset] == 0x5F && apdu[dec.offset + 1] == 0x1F) {
                        dec.offset += 2;
                        uint32_t conf_len;
                        dlms_axdr_decode_length(&dec, &conf_len);
                        if (conf_len >= 4 && dec.offset + 4 <= end_be) {
                            /* Bitstring: byte 0 is unused bits (usually 0), bytes 1..3 is 24-bit conformance */
                            dec.offset++; /* Skip unused bits */
                            params->proposed_conformance =
                                ((uint32_t)apdu[dec.offset] << 16) |
                                ((uint32_t)apdu[dec.offset + 1] << 8) |
                                 (uint32_t)apdu[dec.offset + 2];
                            dec.offset += 3;
                        }
                    } else if (apdu[dec.offset] == 0x02 && apdu[dec.offset + 1] == 0x02) {
                        /* Max PDU size */
                        params->proposed_max_pdu_size =
                            ((uint16_t)apdu[dec.offset + 2] << 8) | apdu[dec.offset + 3];
                        dec.offset += 4;
                    } else {
                        dec.offset++;
                    }
                }
            }
            dec.offset = end_be;
        } else {
            dec.offset += len;
        }
    }

    return true;
}

dlms_result_t dlms_association_process_aarq(
    dlms_association_t *assoc,
    const dlms_aarq_params_t *aarq,
    uint8_t *out_aare, uint16_t max_out_len, uint16_t *actual_len)
{
    if (!assoc || !aarq || !out_aare || !actual_len) return DLMS_ERR_INVALID_PARAM;

    /* Negotiate Conformance and Max PDU size */
    assoc->conformance = aarq->proposed_conformance & DLMS_DEFAULT_SERVER_CONFORMANCE;
    assoc->max_pdu_size = (aarq->proposed_max_pdu_size < DLMS_MAX_PDU_SIZE) ?
                           aarq->proposed_max_pdu_size : DLMS_MAX_PDU_SIZE;
    if (assoc->max_pdu_size < 128) assoc->max_pdu_size = 128;

    uint8_t result = 0; /* 0 = Accepted */
    uint8_t diagnostic = 0; /* 0 = Null */

    /* Check Authentication Mechanism */
    if (aarq->auth_mechanism == DLMS_AUTH_NONE) {
        /* Public Client: Accepted immediately */
        assoc->state = DLMS_ASSOC_STATE_ASSOCIATED;
        assoc->auth_mechanism = DLMS_AUTH_NONE;
    } else if (aarq->auth_mechanism == DLMS_AUTH_LLS) {
        /* Low Level Security (Password check) */
        assoc->auth_mechanism = DLMS_AUTH_LLS;
        if (aarq->calling_auth_len > 0 &&
            strncmp((const char *)aarq->calling_auth_val, DLMS_DEFAULT_LLS_PASSWORD, aarq->calling_auth_len) == 0)
        {
            assoc->state = DLMS_ASSOC_STATE_ASSOCIATED;
        } else {
            result = 1; /* Rejected permanent */
            diagnostic = 1; /* No reason given / auth failure */
            assoc->state = DLMS_ASSOC_STATE_NON_ASSOCIATED;
        }
    } else if (aarq->auth_mechanism == DLMS_AUTH_HLS_GMAC) {
        /* High Level Security 5 (Suite 0 GMAC) */
        assoc->auth_mechanism = DLMS_AUTH_HLS_GMAC;
        /* Store CtoS challenge sent by client */
        if (aarq->calling_auth_len >= DLMS_HLS_CHALLENGE_SIZE) {
            memcpy(assoc->security_ctx.c_to_s_challenge, aarq->calling_auth_val, DLMS_HLS_CHALLENGE_SIZE);
        } else {
            memset(assoc->security_ctx.c_to_s_challenge, 0, DLMS_HLS_CHALLENGE_SIZE);
        }

        /* Generate StoC challenge */
        dlms_random_get_bytes(assoc->security_ctx.s_to_c_challenge, DLMS_HLS_CHALLENGE_SIZE);
        assoc->security_ctx.challenge_pending = true;

        /* Association is accepted, but pending HLS verification! */
        assoc->state = DLMS_ASSOC_STATE_PENDING_HLS;
    } else {
        result = 1;
        diagnostic = 2; /* Application context / mech not supported */
        assoc->state = DLMS_ASSOC_STATE_NON_ASSOCIATED;
    }

    /* Build AARE APDU */
    dlms_axdr_encoder_t enc;
    dlms_axdr_encoder_init(&enc, out_aare, max_out_len);

    out_aare[enc.offset++] = DLMS_TAG_AARE;
    uint16_t len_placeholder = enc.offset;
    enc.offset += 1; /* Reserve 1 byte for BER length (body is <= 127 bytes) */

    /* [0] Application Context Name */
    out_aare[enc.offset++] = 0xA1;
    out_aare[enc.offset++] = 0x09;
    out_aare[enc.offset++] = 0x06;
    out_aare[enc.offset++] = 0x07;
    memcpy(out_aare + enc.offset, oid_app_ctx_prefix, sizeof(oid_app_ctx_prefix));
    enc.offset += sizeof(oid_app_ctx_prefix);
    out_aare[enc.offset++] = (aarq->app_context == DLMS_APP_CONTEXT_LN_CIPHER) ?
                              DLMS_APP_CONTEXT_LN_CIPHER : DLMS_APP_CONTEXT_LN_NO_CIPHER;

    /* [1] Result: INTEGER */
    out_aare[enc.offset++] = 0xA2;
    out_aare[enc.offset++] = 0x03;
    out_aare[enc.offset++] = 0x02; /* INTEGER */
    out_aare[enc.offset++] = 0x01;
    out_aare[enc.offset++] = result;

    /* [2] Result Source Diagnostic */
    out_aare[enc.offset++] = 0xA3;
    out_aare[enc.offset++] = 0x05;
    out_aare[enc.offset++] = 0xA1;
    out_aare[enc.offset++] = 0x03;
    out_aare[enc.offset++] = 0x02;
    out_aare[enc.offset++] = 0x01;
    out_aare[enc.offset++] = diagnostic;

    /* Responding Authentication Value (StoC challenge if HLS 5) */
    if (assoc->auth_mechanism == DLMS_AUTH_HLS_GMAC && result == 0) {
        /* [8] Mechanism name */
        out_aare[enc.offset++] = 0x88;
        out_aare[enc.offset++] = 0x07;
        memcpy(out_aare + enc.offset, oid_auth_mech_prefix, sizeof(oid_auth_mech_prefix));
        enc.offset += sizeof(oid_auth_mech_prefix);
        out_aare[enc.offset++] = DLMS_AUTH_HLS_GMAC;

        /* Responding authentication value */
        out_aare[enc.offset++] = 0xAA;
        out_aare[enc.offset++] = 2 + DLMS_HLS_CHALLENGE_SIZE;
        out_aare[enc.offset++] = 0x80;
        out_aare[enc.offset++] = DLMS_HLS_CHALLENGE_SIZE;
        memcpy(out_aare + enc.offset, assoc->security_ctx.s_to_c_challenge, DLMS_HLS_CHALLENGE_SIZE);
        enc.offset += DLMS_HLS_CHALLENGE_SIZE;
    }

    /* User Information (xDLMS-Initiate-Response) */
    out_aare[enc.offset++] = 0xBE;
    out_aare[enc.offset++] = 0x10; /* Length */
    out_aare[enc.offset++] = 0x04; /* Octet string */
    out_aare[enc.offset++] = 0x0E; /* Octet string length */

    /* InitiateResponse tag = 0x08 */
    out_aare[enc.offset++] = 0x08;
    out_aare[enc.offset++] = 0x00; /* Negotiated quality of service */
    out_aare[enc.offset++] = 0x06; /* DLMS Version 6 */

    /* Conformance (3 bytes bitstring) */
    out_aare[enc.offset++] = 0x5F;
    out_aare[enc.offset++] = 0x1F;
    out_aare[enc.offset++] = 0x04;
    out_aare[enc.offset++] = 0x00;
    out_aare[enc.offset++] = (uint8_t)(assoc->conformance >> 16);
    out_aare[enc.offset++] = (uint8_t)(assoc->conformance >> 8);
    out_aare[enc.offset++] = (uint8_t)(assoc->conformance & 0xFF);

    /* Max PDU size */
    out_aare[enc.offset++] = (uint8_t)(assoc->max_pdu_size >> 8);
    out_aare[enc.offset++] = (uint8_t)(assoc->max_pdu_size & 0xFF);

    /* VAA Name (0x00 0x07) */
    out_aare[enc.offset++] = 0x00;
    out_aare[enc.offset++] = 0x07;

    /* Fill total BER length */
    uint16_t total_body_len = enc.offset - (len_placeholder + 1);
    if (total_body_len <= 0x7F) {
        out_aare[len_placeholder] = (uint8_t)total_body_len;
    } else {
        /* If length ever exceeds 127 bytes, shift body to insert 0x81 */
        memmove(out_aare + len_placeholder + 2, out_aare + len_placeholder + 1, total_body_len);
        out_aare[len_placeholder] = 0x81;
        out_aare[len_placeholder + 1] = (uint8_t)total_body_len;
        enc.offset += 1;
    }

    *actual_len = enc.offset;
    return DLMS_OK;
}

dlms_result_t dlms_association_process_rlrq(
    dlms_association_t *assoc,
    const uint8_t *in_rlrq, uint16_t in_len,
    uint8_t *out_rlre, uint16_t max_out_len, uint16_t *actual_len)
{
    (void)in_rlrq;
    (void)in_len;
    if (!assoc || !out_rlre || !actual_len || max_out_len < 5) return DLMS_ERR_INVALID_PARAM;

    assoc->state = DLMS_ASSOC_STATE_NON_ASSOCIATED;

    /* RLRE format: 0x63 0x03 0x80 0x01 0x00 (Success) */
    out_rlre[0] = DLMS_TAG_RLRE;
    out_rlre[1] = 0x03;
    out_rlre[2] = 0x80;
    out_rlre[3] = 0x01;
    out_rlre[4] = 0x00;

    *actual_len = 5;
    return DLMS_OK;
}
