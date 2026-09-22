/**
 * @file dlms_security_suite0.c
 * @brief DLMS Security Suite 0 Engine Implementation
 */

#include "dlms_security_suite0.h"
#include "dlms_aes_gcm.h"
#include "dlms_aes_keywrap.h"
#include "dlms_axdr.h"
#include <string.h>

dlms_result_t dlms_suite0_encrypt_apdu(
    dlms_security_context_t *sec,
    uint8_t sc,
    dlms_apdu_tag_t cipher_tag,
    const uint8_t *plain_apdu, uint16_t plain_len,
    uint8_t *out_apdu, uint16_t max_out_len, uint16_t *out_len)
{
    if (!sec || !plain_apdu || !out_apdu || !out_len) return DLMS_ERR_INVALID_PARAM;

    sec->server_invocation_counter++;
    uint32_t ic = sec->server_invocation_counter;

    /* Construct IV (12 bytes) = Server System Title (8 bytes) || Invocation Counter (4 bytes) */
    uint8_t iv[12];
    memcpy(iv, sec->server_system_title, 8);
    iv[8]  = (uint8_t)(ic >> 24);
    iv[9]  = (uint8_t)(ic >> 16);
    iv[10] = (uint8_t)(ic >> 8);
    iv[11] = (uint8_t)(ic & 0xFF);

    /* Construct AAD according to Green Book Sec 9.2.7.2 */
    uint8_t aad[64];
    size_t aad_len = 0;

    if ((sc & DLMS_SC_AUTH) && (sc & DLMS_SC_ENC)) {
        /* Authenticated Encryption: AAD = SC || AK */
        aad[0] = sc;
        memcpy(aad + 1, sec->authentication_key, 16);
        aad_len = 17;
    } else if (sc & DLMS_SC_AUTH) {
        /* Authentication Only: AAD = SC || AK || Plaintext */
        aad[0] = sc;
        memcpy(aad + 1, sec->authentication_key, 16);
        aad_len = 17;
        /* Plaintext will be authenticated via GMAC */
    } else {
        /* Encryption Only: AAD = NULL */
        aad_len = 0;
    }

    /* Buffer for ciphertext and tag */
    uint8_t cipher_buf[DLMS_MAX_PDU_SIZE];
    uint8_t tag[DLMS_AUTH_TAG_SIZE];

    if ((sc & DLMS_SC_ENC) != 0) {
        dlms_aes_gcm_encrypt_and_tag(
            sec->global_unicast_key,
            iv, 12,
            (aad_len > 0) ? aad : NULL, aad_len,
            plain_apdu, plain_len,
            cipher_buf,
            tag, DLMS_AUTH_TAG_SIZE);
    } else if ((sc & DLMS_SC_AUTH) != 0) {
        /* Auth only: copy plaintext directly, GMAC over AAD + plain */
        memcpy(cipher_buf, plain_apdu, plain_len);
        dlms_gcm_ctx_t gcm_ctx;
        dlms_gcm_init(&gcm_ctx, sec->authentication_key, iv, 12);
        dlms_gcm_aad(&gcm_ctx, aad, aad_len);
        dlms_gcm_aad(&gcm_ctx, plain_apdu, plain_len);
        dlms_gcm_finish(&gcm_ctx, tag, DLMS_AUTH_TAG_SIZE);
    } else {
        /* Plain copy */
        memcpy(cipher_buf, plain_apdu, plain_len);
    }

    /* Build final output APDU */
    dlms_axdr_encoder_t enc;
    dlms_axdr_encoder_init(&enc, out_apdu, max_out_len);

    /* APDU Tag */
    out_apdu[enc.offset++] = (uint8_t)cipher_tag;

    /* Calculate security payload length */
    uint32_t sec_payload_len = 1 + 4 + plain_len; /* SC(1) + IC(4) + Ciphertext */
    if ((sc & DLMS_SC_AUTH) != 0) {
        sec_payload_len += DLMS_AUTH_TAG_SIZE; /* + Tag(12) */
    }
    if (cipher_tag == DLMS_TAG_GENERAL_GLO_CIPHERING || cipher_tag == DLMS_TAG_GENERAL_DED_CIPHERING) {
        sec_payload_len += (1 + 8); /* Octet-string length(1) + System Title(8) */
    }

    dlms_axdr_encode_length(&enc, sec_payload_len);

    /* If general-glo-ciphering, include Server System Title as octet-string */
    if (cipher_tag == DLMS_TAG_GENERAL_GLO_CIPHERING || cipher_tag == DLMS_TAG_GENERAL_DED_CIPHERING) {
        out_apdu[enc.offset++] = 8;
        memcpy(out_apdu + enc.offset, sec->server_system_title, 8);
        enc.offset += 8;
    }

    /* Security Control */
    out_apdu[enc.offset++] = sc;

    /* Invocation Counter (4 bytes) */
    out_apdu[enc.offset++] = (uint8_t)(ic >> 24);
    out_apdu[enc.offset++] = (uint8_t)(ic >> 16);
    out_apdu[enc.offset++] = (uint8_t)(ic >> 8);
    out_apdu[enc.offset++] = (uint8_t)(ic & 0xFF);

    /* Ciphertext */
    memcpy(out_apdu + enc.offset, cipher_buf, plain_len);
    enc.offset += plain_len;

    /* Tag (if authenticated) */
    if ((sc & DLMS_SC_AUTH) != 0) {
        memcpy(out_apdu + enc.offset, tag, DLMS_AUTH_TAG_SIZE);
        enc.offset += DLMS_AUTH_TAG_SIZE;
    }

    *out_len = enc.offset;
    return DLMS_OK;
}

dlms_result_t dlms_suite0_decrypt_apdu(
    dlms_security_context_t *sec,
    const uint8_t *in_apdu, uint16_t in_len,
    uint8_t *out_plain, uint16_t max_plain_len, uint16_t *plain_len)
{
    if (!sec || !in_apdu || !out_plain || !plain_len) return DLMS_ERR_INVALID_PARAM;
    if (in_len < 7) return DLMS_ERR_FRAME_INCOMPLETE;

    dlms_axdr_decoder_t dec;
    dlms_axdr_decoder_init(&dec, in_apdu, in_len);

    uint8_t tag = in_apdu[dec.offset++];
    uint32_t payload_len = 0;
    if (!dlms_axdr_decode_length(&dec, &payload_len)) return DLMS_ERR_INVALID_PARAM;

    /* If general-glo-ciphering, parse System Title */
    if (tag == DLMS_TAG_GENERAL_GLO_CIPHERING || tag == DLMS_TAG_GENERAL_DED_CIPHERING) {
        if (dec.offset >= in_len) return DLMS_ERR_FRAME_INCOMPLETE;
        uint8_t st_len = in_apdu[dec.offset++];
        if (st_len != 8 || dec.offset + 8 > in_len) return DLMS_ERR_INVALID_PARAM;
        memcpy(sec->client_system_title, in_apdu + dec.offset, 8);
        dec.offset += 8;
    }

    if (dec.offset + 5 > in_len) return DLMS_ERR_FRAME_INCOMPLETE;

    /* Security Control */
    uint8_t sc = in_apdu[dec.offset++];
    if ((sc & DLMS_SC_SUITE_MASK) != DLMS_SECURITY_SUITE_0) {
        return DLMS_ERR_NOT_SUPPORTED; /* Only Security Suite 0 supported */
    }

    /* Invocation Counter */
    uint32_t rx_ic = ((uint32_t)in_apdu[dec.offset] << 24) |
                     ((uint32_t)in_apdu[dec.offset + 1] << 16) |
                     ((uint32_t)in_apdu[dec.offset + 2] << 8) |
                      (uint32_t)in_apdu[dec.offset + 3];
    dec.offset += 4;

    /* Replay attack verification */
    if (sec->client_invocation_counter != 0 && rx_ic <= sec->client_invocation_counter) {
        return DLMS_ERR_REPLAY_ATTACK;
    }
    sec->client_invocation_counter = rx_ic;

    /* Construct IV */
    uint8_t iv[12];
    memcpy(iv, sec->client_system_title, 8);
    iv[8]  = (uint8_t)(rx_ic >> 24);
    iv[9]  = (uint8_t)(rx_ic >> 16);
    iv[10] = (uint8_t)(rx_ic >> 8);
    iv[11] = (uint8_t)(rx_ic & 0xFF);

    /* Construct AAD */
    uint8_t aad[64];
    size_t aad_len = 0;
    if ((sc & DLMS_SC_AUTH) && (sc & DLMS_SC_ENC)) {
        aad[0] = sc;
        memcpy(aad + 1, sec->authentication_key, 16);
        aad_len = 17;
    } else if (sc & DLMS_SC_AUTH) {
        aad[0] = sc;
        memcpy(aad + 1, sec->authentication_key, 16);
        aad_len = 17;
    }

    /* Data length excluding Tag */
    uint16_t tag_len = (sc & DLMS_SC_AUTH) ? DLMS_AUTH_TAG_SIZE : 0;
    if (in_len < dec.offset + tag_len) return DLMS_ERR_FRAME_INCOMPLETE;

    uint16_t cipher_len = in_len - dec.offset - tag_len;
    if (cipher_len > max_plain_len) return DLMS_ERR_BUFFER_OVERFLOW;

    const uint8_t *p_cipher = in_apdu + dec.offset;
    const uint8_t *p_tag = in_apdu + dec.offset + cipher_len;

    if ((sc & DLMS_SC_ENC) != 0) {
        if (!dlms_aes_gcm_decrypt_and_verify(
                sec->global_unicast_key,
                iv, 12,
                (aad_len > 0) ? aad : NULL, aad_len,
                p_cipher, cipher_len,
                p_tag, tag_len,
                out_plain))
        {
            return DLMS_ERR_SECURITY_FAILED;
        }
    } else if ((sc & DLMS_SC_AUTH) != 0) {
        /* Auth only: verify GMAC */
        dlms_gcm_ctx_t gcm_ctx;
        dlms_gcm_init(&gcm_ctx, sec->authentication_key, iv, 12);
        dlms_gcm_aad(&gcm_ctx, aad, aad_len);
        dlms_gcm_aad(&gcm_ctx, p_cipher, cipher_len);
        uint8_t comp_tag[DLMS_AUTH_TAG_SIZE];
        dlms_gcm_finish(&gcm_ctx, comp_tag, DLMS_AUTH_TAG_SIZE);

        uint8_t diff = 0;
        for (int i = 0; i < DLMS_AUTH_TAG_SIZE; i++) diff |= (comp_tag[i] ^ p_tag[i]);
        if (diff != 0) return DLMS_ERR_SECURITY_FAILED;

        memcpy(out_plain, p_cipher, cipher_len);
    } else {
        memcpy(out_plain, p_cipher, cipher_len);
    }

    *plain_len = cipher_len;
    return DLMS_OK;
}

bool dlms_suite0_hls5_verify_reply(
    dlms_security_context_t *sec,
    const uint8_t *client_reply, uint16_t reply_len)
{
    /* Client reply: SC(1: 0x10) || IC(4) || GMAC Tag(12) = 17 bytes */
    if (!sec || !client_reply || reply_len != 17) return false;

    uint8_t sc = client_reply[0];
    if (sc != DLMS_SC_SUITE0_AUTH_ONLY) return false;

    uint32_t client_ic = ((uint32_t)client_reply[1] << 24) |
                         ((uint32_t)client_reply[2] << 16) |
                         ((uint32_t)client_reply[3] << 8) |
                          (uint32_t)client_reply[4];

    if (sec->client_invocation_counter != 0 && client_ic <= sec->client_invocation_counter) {
        return false; /* Replay attack */
    }
    sec->client_invocation_counter = client_ic;

    /* Build IV = Client System Title (8) || Client IC (4) */
    uint8_t iv[12];
    memcpy(iv, sec->client_system_title, 8);
    iv[8]  = (uint8_t)(client_ic >> 24);
    iv[9]  = (uint8_t)(client_ic >> 16);
    iv[10] = (uint8_t)(client_ic >> 8);
    iv[11] = (uint8_t)(client_ic & 0xFF);

    /* Build AAD = SC(1: 0x10) || AK(16) || StoC(16) = 33 bytes */
    uint8_t aad[33];
    aad[0] = sc;
    memcpy(aad + 1, sec->authentication_key, 16);
    memcpy(aad + 17, sec->s_to_c_challenge, 16);

    uint8_t comp_tag[DLMS_AUTH_TAG_SIZE];
    dlms_aes_gmac(sec->authentication_key, iv, 12, aad, 33, comp_tag, DLMS_AUTH_TAG_SIZE);

    const uint8_t *rx_tag = client_reply + 5;
    uint8_t diff = 0;
    for (int i = 0; i < DLMS_AUTH_TAG_SIZE; i++) {
        diff |= (comp_tag[i] ^ rx_tag[i]);
    }

    if (diff == 0) {
        sec->challenge_pending = false;
        return true;
    }
    return false;
}

bool dlms_suite0_hls5_build_response(
    dlms_security_context_t *sec,
    uint8_t *out_response, uint16_t max_out_len, uint16_t *actual_out_len)
{
    if (!sec || !out_response || max_out_len < 17 || !actual_out_len) return false;

    sec->server_invocation_counter++;
    uint32_t ic = sec->server_invocation_counter;

    uint8_t sc = DLMS_SC_SUITE0_AUTH_ONLY;

    /* Build IV = Server System Title (8) || Server IC (4) */
    uint8_t iv[12];
    memcpy(iv, sec->server_system_title, 8);
    iv[8]  = (uint8_t)(ic >> 24);
    iv[9]  = (uint8_t)(ic >> 16);
    iv[10] = (uint8_t)(ic >> 8);
    iv[11] = (uint8_t)(ic & 0xFF);

    /* Build AAD = SC(1) || AK(16) || CtoS(16) = 33 bytes */
    uint8_t aad[33];
    aad[0] = sc;
    memcpy(aad + 1, sec->authentication_key, 16);
    memcpy(aad + 17, sec->c_to_s_challenge, 16);

    uint8_t tag[DLMS_AUTH_TAG_SIZE];
    dlms_aes_gmac(sec->authentication_key, iv, 12, aad, 33, tag, DLMS_AUTH_TAG_SIZE);

    out_response[0] = sc;
    out_response[1] = (uint8_t)(ic >> 24);
    out_response[2] = (uint8_t)(ic >> 16);
    out_response[3] = (uint8_t)(ic >> 8);
    out_response[4] = (uint8_t)(ic & 0xFF);
    memcpy(out_response + 5, tag, DLMS_AUTH_TAG_SIZE);

    *actual_out_len = 17;
    return true;
}

bool dlms_suite0_unwrap_key(
    const dlms_security_context_t *sec,
    const uint8_t *wrapped_key, uint16_t wrapped_len,
    uint8_t *unwrapped_key)
{
    if (!sec || !wrapped_key || !unwrapped_key) return false;
    return dlms_aes_key_unwrap(sec->master_key, wrapped_key, wrapped_len, unwrapped_key);
}
