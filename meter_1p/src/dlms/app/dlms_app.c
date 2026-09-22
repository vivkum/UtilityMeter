/**
 * @file dlms_app.c
 * @brief DLMS/COSEM Application Engine Implementation
 */

#include "dlms_app.h"
#include "dlms_association.h"
#include "dlms_objects.h"
#include "dlms_security_suite0.h"
#include "dlms_meter_data_adapter.h"
#include <string.h>

/* Associations Contexts: Public Client (0x10) and Management Client (0x01) */
static dlms_association_t s_assoc_public;
static dlms_association_t s_assoc_management;

/* HDLC Serial Channel */
static dlms_hdlc_channel_t s_hdlc_channel;

/* HDLC Transmit callback */
static dlms_tx_callback_fn s_tx_cb = NULL;
static void               *s_tx_user_data = NULL;

/* Static keys definition */
static const uint8_t s_default_unicast_key[16] = DLMS_DEFAULT_GLOBAL_UNICAST_KEY;
static const uint8_t s_default_auth_key[16]    = DLMS_DEFAULT_AUTHENTICATION_KEY;
static const uint8_t s_default_master_key[16]  = DLMS_DEFAULT_MASTER_KEY;
static const uint8_t s_server_sys_title[8]     = DLMS_SERVER_SYSTEM_TITLE;
static const uint8_t s_default_client_st[8]    = DLMS_DEFAULT_CLIENT_SYSTEM_TITLE;

static void init_security_context(dlms_security_context_t *sec) {
    memset(sec, 0, sizeof(dlms_security_context_t));
    memcpy(sec->global_unicast_key, s_default_unicast_key, 16);
    memcpy(sec->authentication_key, s_default_auth_key, 16);
    memcpy(sec->master_key, s_default_master_key, 16);
    memcpy(sec->server_system_title, s_server_sys_title, 8);
    memcpy(sec->client_system_title, s_default_client_st, 8);
    sec->security_suite = DLMS_SECURITY_SUITE_0;
    sec->security_policy = DLMS_SEC_POLICY_NOTHING; /* Default open; can be locked down via Security Setup */
    sec->server_invocation_counter = 1000;
    sec->client_invocation_counter = 0;
}

dlms_result_t dlms_init(void) {
    /* Initialize meter adapter */
    dlms_meter_data_adapter_init();

    /* Initialize COSEM objects */
    dlms_objects_init();

    /* Initialize HDLC channel */
    dlms_hdlc_init(&s_hdlc_channel, DLMS_HDLC_SERVER_LOGICAL_ADDR, DLMS_HDLC_SERVER_PHYSICAL_ADDR);

    /* Initialize Public Association */
    memset(&s_assoc_public, 0, sizeof(dlms_association_t));
    s_assoc_public.client_sap = DLMS_SAP_PUBLIC_CLIENT;
    s_assoc_public.server_sap = DLMS_HDLC_SERVER_LOGICAL_ADDR;
    s_assoc_public.auth_mechanism = DLMS_AUTH_NONE;
    s_assoc_public.state = DLMS_ASSOC_STATE_NON_ASSOCIATED;
    s_assoc_public.conformance = DLMS_DEFAULT_SERVER_CONFORMANCE;
    s_assoc_public.max_pdu_size = DLMS_MAX_PDU_SIZE;
    init_security_context(&s_assoc_public.security_ctx);

    /* Initialize Management Association (Suite 0) */
    memset(&s_assoc_management, 0, sizeof(dlms_association_t));
    s_assoc_management.client_sap = DLMS_SAP_MANAGEMENT_CLIENT;
    s_assoc_management.server_sap = DLMS_HDLC_SERVER_LOGICAL_ADDR;
    s_assoc_management.auth_mechanism = DLMS_AUTH_HLS_GMAC;
    s_assoc_management.state = DLMS_ASSOC_STATE_NON_ASSOCIATED;
    s_assoc_management.conformance = DLMS_DEFAULT_SERVER_CONFORMANCE;
    s_assoc_management.max_pdu_size = DLMS_MAX_PDU_SIZE;
    init_security_context(&s_assoc_management.security_ctx);

    return DLMS_OK;
}

void dlms_tasks(void) {
    /* Background tasks: Periodic profile capture, session timeout checks */
}

void dlms_register_hdlc_tx_callback(dlms_tx_callback_fn tx_cb, void *user_data) {
    s_tx_cb = tx_cb;
    s_tx_user_data = user_data;
}

dlms_association_t *dlms_app_get_association(uint8_t client_sap) {
    if (client_sap == DLMS_SAP_MANAGEMENT_CLIENT) {
        return &s_assoc_management;
    }
    return &s_assoc_public;
}

dlms_security_context_t *dlms_get_security_context(uint8_t client_sap) {
    dlms_association_t *assoc = dlms_app_get_association(client_sap);
    return assoc ? &assoc->security_ctx : NULL;
}

dlms_result_t dlms_app_process_apdu(
    uint8_t client_sap,
    const uint8_t *in_apdu, uint16_t in_len,
    uint8_t *out_apdu, uint16_t max_out_len, uint16_t *out_len)
{
    if (!in_apdu || in_len < 1 || !out_apdu || !out_len) return DLMS_ERR_INVALID_PARAM;

    dlms_association_t *assoc = dlms_app_get_association(client_sap);
    uint8_t tag = in_apdu[0];

    /* 1. Association Request (AARQ) */
    if (tag == DLMS_TAG_AARQ) {
        dlms_aarq_params_t aarq;
        if (!dlms_association_parse_aarq(in_apdu, in_len, &aarq)) {
            return DLMS_ERR_INVALID_PARAM;
        }
        return dlms_association_process_aarq(assoc, &aarq, out_apdu, max_out_len, out_len);
    }

    /* 2. Release Request (RLRQ) */
    if (tag == DLMS_TAG_RLRQ) {
        return dlms_association_process_rlrq(assoc, in_apdu, in_len, out_apdu, max_out_len, out_len);
    }

    /* 3. Security Suite 0 Ciphered APDUs */
    if (tag == DLMS_TAG_GENERAL_GLO_CIPHERING ||
        tag == DLMS_TAG_GENERAL_DED_CIPHERING ||
        tag == DLMS_TAG_GLO_GET_REQUEST ||
        tag == DLMS_TAG_GLO_SET_REQUEST ||
        tag == DLMS_TAG_GLO_ACTION_REQUEST)
    {
        uint8_t plain_req[DLMS_MAX_PDU_SIZE];
        uint16_t plain_req_len = 0;

        dlms_result_t dec_res = dlms_suite0_decrypt_apdu(
            &assoc->security_ctx, in_apdu, in_len,
            plain_req, sizeof(plain_req), &plain_req_len);

        if (dec_res != DLMS_OK) {
            return dec_res;
        }

        /* Process decrypted plain xDLMS request */
        uint8_t plain_resp[DLMS_MAX_PDU_SIZE];
        uint16_t plain_resp_len = 0;
        dlms_result_t proc_res = dlms_process_request(
            assoc, plain_req, plain_req_len,
            plain_resp, sizeof(plain_resp), &plain_resp_len);

        if (proc_res != DLMS_OK) {
            return proc_res;
        }

        /* Determine response cipher tag */
        dlms_apdu_tag_t resp_tag = DLMS_TAG_GENERAL_GLO_CIPHERING;
        if (tag == DLMS_TAG_GLO_GET_REQUEST) resp_tag = DLMS_TAG_GLO_GET_RESPONSE;
        else if (tag == DLMS_TAG_GLO_SET_REQUEST) resp_tag = DLMS_TAG_GLO_SET_RESPONSE;
        else if (tag == DLMS_TAG_GLO_ACTION_REQUEST) resp_tag = DLMS_TAG_GLO_ACTION_RESPONSE;

        /* Encrypt and authenticate response with Suite 0 */
        return dlms_suite0_encrypt_apdu(
            &assoc->security_ctx,
            DLMS_SC_SUITE0_AUTH_ENC,
            resp_tag,
            plain_resp, plain_resp_len,
            out_apdu, max_out_len, out_len);
    }

    /* 4. Plain xDLMS Request (GET, SET, ACTION) */
    if (tag == DLMS_TAG_GET_REQUEST || tag == DLMS_TAG_SET_REQUEST || tag == DLMS_TAG_ACTION_REQUEST) {
        /* Check Security Policy: if policy mandates encryption and arrived plain, reject */
        if ((assoc->security_ctx.security_policy & DLMS_SEC_POLICY_REQ_ENC) != 0) {
            return DLMS_ERR_SECURITY_FAILED;
        }

        return dlms_process_request(assoc, in_apdu, in_len, out_apdu, max_out_len, out_len);
    }

    return DLMS_ERR_NOT_SUPPORTED;
}

void dlms_hdlc_rx_byte(uint8_t byte) {
    dlms_hdlc_rx_frame_t frame;
    if (dlms_hdlc_feed_byte(&s_hdlc_channel, byte, &frame)) {
        uint8_t tx_buf[DLMS_HDLC_TX_BUF_SIZE];
        uint16_t tx_len = 0;

        if (frame.type == DLMS_HDLC_FRAME_SNRM) {
            /* Respond with UA */
            s_hdlc_channel.connected = true;
            s_hdlc_channel.client_sap = frame.src_address;
            s_hdlc_channel.send_seq = 0;
            s_hdlc_channel.recv_seq = 0;

            tx_len = dlms_hdlc_build_ua(&s_hdlc_channel, frame.src_address, tx_buf, sizeof(tx_buf));
            if (tx_len > 0 && s_tx_cb) {
                s_tx_cb(tx_buf, tx_len, s_tx_user_data);
            }
        } else if (frame.type == DLMS_HDLC_FRAME_DISC) {
            /* Respond with UA and disconnect */
            tx_len = dlms_hdlc_build_ua(&s_hdlc_channel, frame.src_address, tx_buf, sizeof(tx_buf));
            s_hdlc_channel.connected = false;
            dlms_association_t *assoc = dlms_app_get_association(frame.src_address);
            if (assoc) assoc->state = DLMS_ASSOC_STATE_NON_ASSOCIATED;

            if (tx_len > 0 && s_tx_cb) {
                s_tx_cb(tx_buf, tx_len, s_tx_user_data);
            }
        } else if (frame.type == DLMS_HDLC_FRAME_I && frame.info_payload && frame.info_len > 0) {
            /* Update receive sequence number */
            s_hdlc_channel.recv_seq = (frame.n_s + 1) & 0x07;

            /* Process APDU */
            uint8_t resp_apdu[DLMS_MAX_PDU_SIZE];
            uint16_t resp_apdu_len = 0;
            dlms_result_t res = dlms_app_process_apdu(
                frame.src_address,
                frame.info_payload, frame.info_len,
                resp_apdu, sizeof(resp_apdu), &resp_apdu_len);

            if (res == DLMS_OK && resp_apdu_len > 0) {
                tx_len = dlms_hdlc_build_iframe(
                    &s_hdlc_channel, frame.src_address,
                    resp_apdu, resp_apdu_len,
                    tx_buf, sizeof(tx_buf));

                if (tx_len > 0 && s_tx_cb) {
                    s_tx_cb(tx_buf, tx_len, s_tx_user_data);
                }
            }
        }
    }
}

void dlms_hdlc_rx_buffer(const uint8_t *data, uint16_t length) {
    if (!data) return;
    for (uint16_t i = 0; i < length; i++) {
        dlms_hdlc_rx_byte(data[i]);
    }
}

dlms_result_t dlms_wrapper_process(
    const uint8_t *in_data, uint16_t in_len,
    uint8_t *out_resp, uint16_t max_resp_len, uint16_t *actual_resp_len)
{
    dlms_wrapper_packet_t pkt;
    if (!dlms_wrapper_parse(in_data, in_len, &pkt)) {
        return DLMS_ERR_INVALID_PARAM;
    }

    uint8_t resp_apdu[DLMS_MAX_PDU_SIZE];
    uint16_t resp_apdu_len = 0;

    dlms_result_t res = dlms_app_process_apdu(
        (uint8_t)pkt.source_wport,
        pkt.apdu, pkt.length,
        resp_apdu, sizeof(resp_apdu), &resp_apdu_len);

    if (res != DLMS_OK) return res;

    *actual_resp_len = dlms_wrapper_build_response(
        pkt.destination_wport,
        pkt.source_wport,
        resp_apdu, resp_apdu_len,
        out_resp, max_resp_len);

    return DLMS_OK;
}
