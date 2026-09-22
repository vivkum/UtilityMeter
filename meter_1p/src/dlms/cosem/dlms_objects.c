/**
 * @file dlms_objects.c
 * @brief COSEM Object Dictionary and xDLMS Services Implementation
 */

#include "dlms_objects.h"
#include "dlms_security_suite0.h"
#include "dlms_axdr.h"
#include <string.h>
#include <stdio.h>

/* ========================================================================== */
/* Internal Profile Generic Buffer                                           */
/* ========================================================================== */

typedef struct {
    dlms_date_time_t timestamp;
    double           active_energy;
    float            voltage_a;
    float            voltage_b;
    float            voltage_c;
    float            current_a;
    float            current_b;
    float            current_c;
} dlms_load_profile_entry_t;

static dlms_load_profile_entry_t s_load_profile_buffer[DLMS_LOAD_PROFILE_MAX_ENTRIES];
static uint16_t s_load_profile_count = 0;
static uint16_t s_load_profile_head = 0;

/* Block Transfer Context for Long GET responses */
static struct {
    uint32_t current_block_num;
    uint8_t  cache_buf[DLMS_MAX_PDU_SIZE * 2];
    uint16_t cache_len;
    uint16_t sent_offset;
    bool     active;
} s_block_transfer;

/* Relay state */
static bool s_relay_connected = true;

/* ========================================================================== */
/* Attribute Read & Write Callback Functions                                 */
/* ========================================================================== */

/* --- Class 1: Data Callbacks --- */
static dlms_data_access_result_t data_read_fn(
    const dlms_cosem_object_t *obj, uint8_t attr_index,
    uint8_t *out_buf, uint16_t max_out_len, uint16_t *actual_len, void *ctx)
{
    (void)ctx;
    dlms_axdr_encoder_t enc;
    dlms_axdr_encoder_init(&enc, out_buf, max_out_len);

    if (attr_index == 1) {
        /* Logical Name (OBIS) */
        uint8_t obis_bytes[6] = { obj->obis.a, obj->obis.b, obj->obis.c, obj->obis.d, obj->obis.e, obj->obis.f };
        dlms_axdr_encode_octet_string(&enc, obis_bytes, 6);
    } else if (attr_index == 2) {
        /* Value */
        if (obj->obis.c == 42) {
            /* Logical Device Name */
            dlms_axdr_encode_visible_string(&enc, DLMS_DEVICE_NAME);
        } else if (obj->obis.c == 96 && obj->obis.d == 1 && obj->obis.e == 0) {
            /* Serial Number */
            const dlms_meter_adapter_t *ad = dlms_meter_adapter_get();
            const char *sn = (ad && ad->get_serial_number) ? ad->get_serial_number() : "PIC32CX00001";
            dlms_axdr_encode_visible_string(&enc, sn);
        } else if (obj->obis.c == 96 && obj->obis.d == 1 && obj->obis.e == 1) {
            /* Manufacturer */
            dlms_axdr_encode_visible_string(&enc, DLMS_METER_MANUFACTURER_ID);
        } else if (obj->obis.c == 96 && obj->obis.d == 1 && obj->obis.e == 2) {
            /* Firmware Version */
            dlms_axdr_encode_visible_string(&enc, "v2.0.0-Suite0");
        } else {
            return DLMS_RESULT_OBJECT_UNDEFINED;
        }
    } else {
        return DLMS_RESULT_READ_WRITE_DENIED;
    }

    *actual_len = enc.offset;
    return DLMS_RESULT_SUCCESS;
}

/* --- Class 3: Register Callbacks --- */
static dlms_data_access_result_t register_read_fn(
    const dlms_cosem_object_t *obj, uint8_t attr_index,
    uint8_t *out_buf, uint16_t max_out_len, uint16_t *actual_len, void *ctx)
{
    (void)ctx;
    const dlms_meter_adapter_t *ad = dlms_meter_adapter_get();
    dlms_meter_instantaneous_t inst = {0};
    dlms_meter_energy_t energy = {0};

    if (ad && ad->get_instantaneous) ad->get_instantaneous(&inst);
    if (ad && ad->get_energy) ad->get_energy(&energy);

    dlms_axdr_encoder_t enc;
    dlms_axdr_encoder_init(&enc, out_buf, max_out_len);

    if (attr_index == 1) {
        /* Logical name */
        uint8_t obis_bytes[6] = { obj->obis.a, obj->obis.b, obj->obis.c, obj->obis.d, obj->obis.e, obj->obis.f };
        dlms_axdr_encode_octet_string(&enc, obis_bytes, 6);
    } else if (attr_index == 2) {
        /* Value */
        if (obj->obis.c == 1 && obj->obis.d == 8) {
            /* Active Energy Import (+A): Wh */
            dlms_axdr_encode_u32(&enc, (uint32_t)energy.active_energy_import);
        } else if (obj->obis.c == 2 && obj->obis.d == 8) {
            /* Active Energy Export (-A): Wh */
            dlms_axdr_encode_u32(&enc, (uint32_t)energy.active_energy_export);
        } else if (obj->obis.c == 3 && obj->obis.d == 8) {
            /* Reactive Energy Import (+R): varh */
            dlms_axdr_encode_u32(&enc, (uint32_t)energy.reactive_energy_import);
        } else if (obj->obis.c == 4 && obj->obis.d == 8) {
            /* Reactive Energy Export (-R): varh */
            dlms_axdr_encode_u32(&enc, (uint32_t)energy.reactive_energy_export);
        } else if (obj->obis.c == 9 && obj->obis.d == 8) {
            /* Apparent Energy Import (+S): VAh */
            dlms_axdr_encode_u32(&enc, (uint32_t)energy.apparent_energy_import);
        } else if (obj->obis.c == 1 && obj->obis.d == 7) {
            /* Active Power Total (W) */
            dlms_axdr_encode_u32(&enc, (uint32_t)inst.active_power_total);
        } else if (obj->obis.c == 21 && obj->obis.d == 7) {
            /* Active Power Phase A (W) */
            dlms_axdr_encode_u32(&enc, (uint32_t)inst.active_power_a);
        } else if (obj->obis.c == 41 && obj->obis.d == 7) {
            /* Active Power Phase B (W) */
            dlms_axdr_encode_u32(&enc, (uint32_t)inst.active_power_b);
        } else if (obj->obis.c == 61 && obj->obis.d == 7) {
            /* Active Power Phase C (W) */
            dlms_axdr_encode_u32(&enc, (uint32_t)inst.active_power_c);
        } else if (obj->obis.c == 32 && obj->obis.d == 7) {
            /* Voltage Phase A: Scaler -1 (0.1 V) */
            dlms_axdr_encode_u16(&enc, (uint16_t)(inst.voltage_a * 10.0f));
        } else if (obj->obis.c == 52 && obj->obis.d == 7) {
            /* Voltage Phase B */
            dlms_axdr_encode_u16(&enc, (uint16_t)(inst.voltage_b * 10.0f));
        } else if (obj->obis.c == 72 && obj->obis.d == 7) {
            /* Voltage Phase C */
            dlms_axdr_encode_u16(&enc, (uint16_t)(inst.voltage_c * 10.0f));
        } else if (obj->obis.c == 31 && obj->obis.d == 7) {
            /* Current Phase A: Scaler -2 (0.01 A) */
            dlms_axdr_encode_u16(&enc, (uint16_t)(inst.current_a * 100.0f));
        } else if (obj->obis.c == 51 && obj->obis.d == 7) {
            /* Current Phase B */
            dlms_axdr_encode_u16(&enc, (uint16_t)(inst.current_b * 100.0f));
        } else if (obj->obis.c == 71 && obj->obis.d == 7) {
            /* Current Phase C */
            dlms_axdr_encode_u16(&enc, (uint16_t)(inst.current_c * 100.0f));
        } else if (obj->obis.c == 14 && obj->obis.d == 7) {
            /* Frequency: Scaler -2 (0.01 Hz) */
            dlms_axdr_encode_u16(&enc, (uint16_t)(inst.frequency * 100.0f));
        } else if (obj->obis.c == 13 && obj->obis.d == 7) {
            /* Power Factor Total: Scaler -3 (0.001) */
            dlms_axdr_encode_i16(&enc, (int16_t)(inst.power_factor * 1000.0f));
        } else {
            return DLMS_RESULT_OBJECT_UNDEFINED;
        }
    } else if (attr_index == 3) {
        /* Scaler_unit */
        int8_t scaler = 0;
        dlms_unit_t unit = DLMS_UNIT_NONE;

        if (obj->obis.d == 8) {
            scaler = 0;
            unit = (obj->obis.c == 3 || obj->obis.c == 4) ? DLMS_UNIT_REACTIVE_ENERGY_VARH :
                   (obj->obis.c == 9) ? DLMS_UNIT_APPARENT_ENERGY_VAH : DLMS_UNIT_ACTIVE_ENERGY_WH;
        } else if (obj->obis.d == 7) {
            if (obj->obis.c == 32 || obj->obis.c == 52 || obj->obis.c == 72) {
                scaler = -1; unit = DLMS_UNIT_VOLTAGE_VOLT;
            } else if (obj->obis.c == 31 || obj->obis.c == 51 || obj->obis.c == 71) {
                scaler = -2; unit = DLMS_UNIT_CURRENT_AMPERE;
            } else if (obj->obis.c == 14) {
                scaler = -2; unit = DLMS_UNIT_FREQUENCY_HERTZ;
            } else if (obj->obis.c == 13) {
                scaler = -3; unit = DLMS_UNIT_COUNT;
            } else {
                scaler = 0; unit = DLMS_UNIT_ACTIVE_POWER_WATT;
            }
        }
        dlms_axdr_encode_scaler_unit(&enc, scaler, unit);
    } else {
        return DLMS_RESULT_READ_WRITE_DENIED;
    }

    *actual_len = enc.offset;
    return DLMS_RESULT_SUCCESS;
}

/* --- Class 4: Extended Register Callbacks (Max Demand) --- */
static dlms_data_access_result_t ext_register_read_fn(
    const dlms_cosem_object_t *obj, uint8_t attr_index,
    uint8_t *out_buf, uint16_t max_out_len, uint16_t *actual_len, void *ctx)
{
    (void)ctx;
    const dlms_meter_adapter_t *ad = dlms_meter_adapter_get();
    dlms_meter_max_demand_t md = {0};
    if (ad && ad->get_max_demand) ad->get_max_demand(&md);

    dlms_axdr_encoder_t enc;
    dlms_axdr_encoder_init(&enc, out_buf, max_out_len);

    if (attr_index == 1) {
        uint8_t obis_bytes[6] = { obj->obis.a, obj->obis.b, obj->obis.c, obj->obis.d, obj->obis.e, obj->obis.f };
        dlms_axdr_encode_octet_string(&enc, obis_bytes, 6);
    } else if (attr_index == 2) {
        /* Value: W */
        dlms_axdr_encode_u32(&enc, (uint32_t)md.value);
    } else if (attr_index == 3) {
        /* Scaler unit */
        dlms_axdr_encode_scaler_unit(&enc, 0, DLMS_UNIT_ACTIVE_POWER_WATT);
    } else if (attr_index == 4) {
        /* Status */
        dlms_axdr_encode_u8(&enc, 0);
    } else if (attr_index == 5) {
        /* Capture time (date_time) */
        dlms_axdr_encode_date_time(&enc, &md.timestamp);
    } else {
        return DLMS_RESULT_READ_WRITE_DENIED;
    }

    *actual_len = enc.offset;
    return DLMS_RESULT_SUCCESS;
}

/* --- Class 8: Clock Callbacks --- */
static dlms_data_access_result_t clock_read_fn(
    const dlms_cosem_object_t *obj, uint8_t attr_index,
    uint8_t *out_buf, uint16_t max_out_len, uint16_t *actual_len, void *ctx)
{
    (void)obj;
    (void)ctx;
    const dlms_meter_adapter_t *ad = dlms_meter_adapter_get();
    dlms_date_time_t dt = { 2026, 9, 17, 4, 12, 0, 0, 0, 330, 0 };
    if (ad && ad->get_rtc_time) ad->get_rtc_time(&dt);

    dlms_axdr_encoder_t enc;
    dlms_axdr_encoder_init(&enc, out_buf, max_out_len);

    if (attr_index == 1) {
        uint8_t obis_bytes[6] = { 0, 0, 1, 0, 0, 255 };
        dlms_axdr_encode_octet_string(&enc, obis_bytes, 6);
    } else if (attr_index == 2) {
        /* Time */
        dlms_axdr_encode_date_time(&enc, &dt);
    } else if (attr_index == 3) {
        /* Time Zone: -720..+720 */
        dlms_axdr_encode_i16(&enc, dt.deviation);
    } else if (attr_index == 4) {
        /* Status */
        dlms_axdr_encode_u8(&enc, dt.clock_status);
    } else {
        return DLMS_RESULT_READ_WRITE_DENIED;
    }

    *actual_len = enc.offset;
    return DLMS_RESULT_SUCCESS;
}

static dlms_data_access_result_t clock_write_fn(
    dlms_cosem_object_t *obj, uint8_t attr_index,
    const uint8_t *in_buf, uint16_t in_len, void *ctx)
{
    (void)obj;
    (void)ctx;
    if (attr_index == 2) {
        dlms_axdr_decoder_t dec;
        dlms_axdr_decoder_init(&dec, in_buf, in_len);
        dlms_date_time_t dt;
        if (dlms_axdr_decode_date_time(&dec, &dt)) {
            const dlms_meter_adapter_t *ad = dlms_meter_adapter_get();
            if (ad && ad->set_rtc_time) {
                ad->set_rtc_time(&dt);
                return DLMS_RESULT_SUCCESS;
            }
        }
        return DLMS_RESULT_TYPE_UNMATCHED;
    }
    return DLMS_RESULT_READ_WRITE_DENIED;
}

/* Forward declaration of object_list encoder */
static dlms_data_access_result_t encode_object_list(
    uint8_t *out_buf, uint16_t max_out_len, uint16_t *actual_len);

/* --- Class 15: Association LN Callbacks --- */
static dlms_data_access_result_t assoc_read_fn(
    const dlms_cosem_object_t *obj, uint8_t attr_index,
    uint8_t *out_buf, uint16_t max_out_len, uint16_t *actual_len, void *ctx)
{
    (void)ctx;
    dlms_axdr_encoder_t enc;
    dlms_axdr_encoder_init(&enc, out_buf, max_out_len);

    if (attr_index == 1) {
        uint8_t obis_bytes[6] = { obj->obis.a, obj->obis.b, obj->obis.c, obj->obis.d, obj->obis.e, obj->obis.f };
        dlms_axdr_encode_octet_string(&enc, obis_bytes, 6);
    } else if (attr_index == 3) {
        /* Associated Partners ID: structure(client_sap, server_sap) */
        dlms_axdr_encode_structure_header(&enc, 2);
        dlms_axdr_encode_i16(&enc, (obj->obis.e <= 1) ? DLMS_SAP_PUBLIC_CLIENT : DLMS_SAP_MANAGEMENT_CLIENT);
        dlms_axdr_encode_u16(&enc, DLMS_HDLC_SERVER_LOGICAL_ADDR);
    } else if (attr_index == 4) {
        /* Application Context Name */
        uint8_t oid[7] = { 0x60, 0x85, 0x74, 0x05, 0x08, 0x01, (obj->obis.e <= 1) ? 1 : 3 };
        dlms_axdr_encode_octet_string(&enc, oid, 7);
    } else if (attr_index == 2) {
        /*
         * Attribute 2: object_list (Association View)
         * Encodes the directory of all COSEM objects exposed to the client.
         */
        return encode_object_list(out_buf, max_out_len, actual_len);
    } else {
        return DLMS_RESULT_READ_WRITE_DENIED;
    }

    *actual_len = enc.offset;
    return DLMS_RESULT_SUCCESS;
}

static dlms_action_result_t assoc_method_fn(
    dlms_cosem_object_t *obj, uint8_t method_index,
    const uint8_t *in_data, uint16_t in_len,
    uint8_t *out_data, uint16_t max_out_len, uint16_t *actual_out_len, void *ctx)
{
    dlms_association_t *assoc = (dlms_association_t *)ctx;
    if (!assoc) return DLMS_ACTION_RESULT_HARDWARE_FAULT;

    if (obj->obis.e == 3 && method_index == 1) {
        /* Method 1: reply_to_HLS_authentication (HLS 5 Suite 0 GMAC) */
        if (!assoc->security_ctx.challenge_pending) {
            return DLMS_ACTION_RESULT_READ_WRITE_DENIED;
        }

        /* Parameter data is an octet-string containing SC(1) + IC(4) + GMAC Tag(12) = 17 bytes */
        dlms_axdr_decoder_t dec;
        dlms_axdr_decoder_init(&dec, in_data, in_len);
        const uint8_t *client_reply;
        uint32_t reply_len;

        if (!dlms_axdr_decode_octet_string(&dec, &client_reply, &reply_len) || reply_len != 17) {
            return DLMS_ACTION_RESULT_TYPE_UNMATCHED;
        }

        /* Verify client's GMAC reply f(StoC) */
        if (!dlms_suite0_hls5_verify_reply(&assoc->security_ctx, client_reply, (uint16_t)reply_len)) {
            assoc->state = DLMS_ASSOC_STATE_NON_ASSOCIATED;
            return DLMS_ACTION_RESULT_READ_WRITE_DENIED;
        }

        /* Success! Client identity authenticated. Transition association to full ASSOCIATED state */
        assoc->state = DLMS_ASSOC_STATE_ASSOCIATED;

        /* Build server's mutual authentication response f(CtoS) */
        uint8_t server_resp[17];
        uint16_t resp_len = 0;
        dlms_suite0_hls5_build_response(&assoc->security_ctx, server_resp, sizeof(server_resp), &resp_len);

        dlms_axdr_encoder_t enc;
        dlms_axdr_encoder_init(&enc, out_data, max_out_len);
        dlms_axdr_encode_octet_string(&enc, server_resp, resp_len);
        *actual_out_len = enc.offset;

        return DLMS_ACTION_RESULT_SUCCESS;
    }

    return DLMS_ACTION_RESULT_READ_WRITE_DENIED;
}

/* --- Class 64: Security Setup Callbacks (Suite 0) --- */
static dlms_data_access_result_t sec_setup_read_fn(
    const dlms_cosem_object_t *obj, uint8_t attr_index,
    uint8_t *out_buf, uint16_t max_out_len, uint16_t *actual_len, void *ctx)
{
    dlms_association_t *assoc = (dlms_association_t *)ctx;
    dlms_axdr_encoder_t enc;
    dlms_axdr_encoder_init(&enc, out_buf, max_out_len);

    if (attr_index == 1) {
        uint8_t obis_bytes[6] = { obj->obis.a, obj->obis.b, obj->obis.c, obj->obis.d, obj->obis.e, obj->obis.f };
        dlms_axdr_encode_octet_string(&enc, obis_bytes, 6);
    } else if (attr_index == 2) {
        /* Security Policy */
        uint8_t policy = assoc ? assoc->security_ctx.security_policy : 0x00;
        dlms_axdr_encode_enum(&enc, policy);
    } else if (attr_index == 3) {
        /* Security Suite: Suite 0 */
        dlms_axdr_encode_enum(&enc, DLMS_SECURITY_SUITE_0);
    } else if (attr_index == 4) {
        /* Client System Title (8 bytes) */
        const uint8_t *st = assoc ? assoc->security_ctx.client_system_title : (const uint8_t *)"";
        dlms_axdr_encode_octet_string(&enc, st, 8);
    } else if (attr_index == 5) {
        /* Server System Title (8 bytes) */
        const uint8_t *st = assoc ? assoc->security_ctx.server_system_title : (const uint8_t *)"";
        dlms_axdr_encode_octet_string(&enc, st, 8);
    } else {
        return DLMS_RESULT_READ_WRITE_DENIED;
    }

    *actual_len = enc.offset;
    return DLMS_RESULT_SUCCESS;
}

static dlms_action_result_t sec_setup_method_fn(
    dlms_cosem_object_t *obj, uint8_t method_index,
    const uint8_t *in_data, uint16_t in_len,
    uint8_t *out_data, uint16_t max_out_len, uint16_t *actual_out_len, void *ctx)
{
    (void)obj;
    (void)out_data;
    (void)max_out_len;
    *actual_out_len = 0;

    dlms_association_t *assoc = (dlms_association_t *)ctx;
    if (!assoc || assoc->state != DLMS_ASSOC_STATE_ASSOCIATED) {
        return DLMS_ACTION_RESULT_READ_WRITE_DENIED;
    }

    if (method_index == 1) {
        /* Security Activate: parameter enum (new security policy) */
        dlms_axdr_decoder_t dec;
        dlms_axdr_decoder_init(&dec, in_data, in_len);
        uint8_t new_policy;
        if (dlms_axdr_decode_enum(&dec, &new_policy)) {
            assoc->security_ctx.security_policy = new_policy;
            return DLMS_ACTION_RESULT_SUCCESS;
        }
        return DLMS_ACTION_RESULT_TYPE_UNMATCHED;
    } else if (method_index == 2) {
        /*
         * Method 2: Key Transfer (IEC 62056-5-3 / Green Book):
         * Parameters: Array of structure { key_id (enum), wrapped_key (octet_string) }
         */
        dlms_axdr_decoder_t dec;
        dlms_axdr_decoder_init(&dec, in_data, in_len);

        uint32_t num_keys;
        if (!dlms_axdr_decode_array_header(&dec, &num_keys)) {
            /* May be sent as single structure */
            uint32_t struct_elems;
            if (!dlms_axdr_decode_structure_header(&dec, &struct_elems) || struct_elems != 2) {
                return DLMS_ACTION_RESULT_TYPE_UNMATCHED;
            }
            num_keys = 1;
        }

        for (uint32_t k = 0; k < num_keys; k++) {
            uint32_t struct_elems;
            dlms_axdr_decode_structure_header(&dec, &struct_elems);
            uint8_t key_id;
            if (!dlms_axdr_decode_enum(&dec, &key_id)) return DLMS_ACTION_RESULT_TYPE_UNMATCHED;

            const uint8_t *wrapped_key;
            uint32_t wrapped_len;
            if (!dlms_axdr_decode_octet_string(&dec, &wrapped_key, &wrapped_len) || wrapped_len != 24) {
                return DLMS_ACTION_RESULT_TYPE_UNMATCHED;
            }

            uint8_t unwrapped_key[16];
            if (!dlms_suite0_unwrap_key(&assoc->security_ctx, wrapped_key, (uint16_t)wrapped_len, unwrapped_key)) {
                return DLMS_ACTION_RESULT_READ_WRITE_DENIED; /* Key verification failed */
            }

            /* Update key */
            if (key_id == 0) {
                /* Global Unicast Encryption Key */
                memcpy(assoc->security_ctx.global_unicast_key, unwrapped_key, 16);
            } else if (key_id == 1) {
                /* Global Broadcast Encryption Key */
                memcpy(assoc->security_ctx.broadcast_key, unwrapped_key, 16);
            } else if (key_id == 2) {
                /* Authentication Key */
                memcpy(assoc->security_ctx.authentication_key, unwrapped_key, 16);
            } else if (key_id == 3) {
                /* Master Key / KEK */
                memcpy(assoc->security_ctx.master_key, unwrapped_key, 16);
            }
        }

        return DLMS_ACTION_RESULT_SUCCESS;
    }

    return DLMS_ACTION_RESULT_READ_WRITE_DENIED;
}

/* --- Class 7: Profile Generic Callbacks (Load Survey) --- */
static dlms_data_access_result_t profile_read_fn(
    const dlms_cosem_object_t *obj, uint8_t attr_index,
    uint8_t *out_buf, uint16_t max_out_len, uint16_t *actual_len, void *ctx)
{
    (void)obj;
    (void)ctx;
    dlms_axdr_encoder_t enc;
    dlms_axdr_encoder_init(&enc, out_buf, max_out_len);

    if (attr_index == 1) {
        uint8_t obis_bytes[6] = { 1, 0, 99, 1, 0, 255 };
        dlms_axdr_encode_octet_string(&enc, obis_bytes, 6);
    } else if (attr_index == 2) {
        /* Buffer: Array of structures */
        uint16_t count = s_load_profile_count;
        if (count > 4) count = 4; /* Limit single block to fit inside PDU */
        dlms_axdr_encode_array_header(&enc, count);
        for (uint16_t i = 0; i < count; i++) {
            const dlms_load_profile_entry_t *e = &s_load_profile_buffer[i];
            dlms_axdr_encode_structure_header(&enc, 8);
            dlms_axdr_encode_date_time(&enc, &e->timestamp);
            dlms_axdr_encode_u32(&enc, (uint32_t)e->active_energy);
            dlms_axdr_encode_u16(&enc, (uint16_t)(e->voltage_a * 10.0f));
            dlms_axdr_encode_u16(&enc, (uint16_t)(e->voltage_b * 10.0f));
            dlms_axdr_encode_u16(&enc, (uint16_t)(e->voltage_c * 10.0f));
            dlms_axdr_encode_u16(&enc, (uint16_t)(e->current_a * 100.0f));
            dlms_axdr_encode_u16(&enc, (uint16_t)(e->current_b * 100.0f));
            dlms_axdr_encode_u16(&enc, (uint16_t)(e->current_c * 100.0f));
        }
    } else if (attr_index == 4) {
        /* Capture period: 900 seconds (15 minutes) */
        dlms_axdr_encode_u32(&enc, 900);
    } else if (attr_index == 7) {
        /* Entries in use */
        dlms_axdr_encode_u32(&enc, s_load_profile_count);
    } else if (attr_index == 8) {
        /* Profile entries capacity */
        dlms_axdr_encode_u32(&enc, DLMS_LOAD_PROFILE_MAX_ENTRIES);
    } else {
        return DLMS_RESULT_READ_WRITE_DENIED;
    }

    *actual_len = enc.offset;
    return DLMS_RESULT_SUCCESS;
}

static dlms_action_result_t profile_method_fn(
    dlms_cosem_object_t *obj, uint8_t method_index,
    const uint8_t *in_data, uint16_t in_len,
    uint8_t *out_data, uint16_t max_out_len, uint16_t *actual_out_len, void *ctx)
{
    (void)obj;
    (void)in_data;
    (void)in_len;
    (void)out_data;
    (void)max_out_len;
    (void)ctx;
    *actual_out_len = 0;

    if (method_index == 1) {
        /* Reset */
        s_load_profile_count = 0;
        s_load_profile_head = 0;
        return DLMS_ACTION_RESULT_SUCCESS;
    } else if (method_index == 2) {
        /* Capture */
        dlms_profile_generic_capture();
        return DLMS_ACTION_RESULT_SUCCESS;
    }
    return DLMS_ACTION_RESULT_READ_WRITE_DENIED;
}

/* --- Class 70: Disconnect Control Callbacks --- */
static dlms_data_access_result_t disconnect_read_fn(
    const dlms_cosem_object_t *obj, uint8_t attr_index,
    uint8_t *out_buf, uint16_t max_out_len, uint16_t *actual_len, void *ctx)
{
    (void)obj;
    (void)ctx;
    const dlms_meter_adapter_t *ad = dlms_meter_adapter_get();
    if (ad && ad->get_relay_state) ad->get_relay_state(&s_relay_connected);

    dlms_axdr_encoder_t enc;
    dlms_axdr_encoder_init(&enc, out_buf, max_out_len);

    if (attr_index == 1) {
        uint8_t obis_bytes[6] = { 0, 0, 96, 3, 10, 255 };
        dlms_axdr_encode_octet_string(&enc, obis_bytes, 6);
    } else if (attr_index == 2) {
        /* Output state: boolean */
        dlms_axdr_encode_bool(&enc, s_relay_connected);
    } else if (attr_index == 3) {
        /* Control state: 0=disconnected, 1=connected, 2=ready */
        dlms_axdr_encode_enum(&enc, s_relay_connected ? 1 : 0);
    } else if (attr_index == 4) {
        /* Control mode: mode 1 */
        dlms_axdr_encode_enum(&enc, 1);
    } else {
        return DLMS_RESULT_READ_WRITE_DENIED;
    }

    *actual_len = enc.offset;
    return DLMS_RESULT_SUCCESS;
}

static dlms_action_result_t disconnect_method_fn(
    dlms_cosem_object_t *obj, uint8_t method_index,
    const uint8_t *in_data, uint16_t in_len,
    uint8_t *out_data, uint16_t max_out_len, uint16_t *actual_out_len, void *ctx)
{
    (void)obj;
    (void)in_data;
    (void)in_len;
    (void)out_data;
    (void)max_out_len;
    (void)ctx;
    *actual_out_len = 0;

    const dlms_meter_adapter_t *ad = dlms_meter_adapter_get();
    if (method_index == 1) {
        /* Remote Disconnect */
        s_relay_connected = false;
        if (ad && ad->set_relay_state) ad->set_relay_state(false);
        return DLMS_ACTION_RESULT_SUCCESS;
    } else if (method_index == 2) {
        /* Remote Reconnect */
        s_relay_connected = true;
        if (ad && ad->set_relay_state) ad->set_relay_state(true);
        return DLMS_ACTION_RESULT_SUCCESS;
    }
    return DLMS_ACTION_RESULT_READ_WRITE_DENIED;
}

/* ========================================================================== */
/* COSEM Object Dictionary Table                                              */
/* ========================================================================== */

static dlms_cosem_object_t s_cosem_objects[] = {
    /* --- Class 1: Data Objects --- */
    { DLMS_CLASS_DATA, 0, DLMS_OBIS_INIT(0, 0, 42, 0, 0, 255), 2, NULL, 0, NULL, data_read_fn, NULL, NULL, NULL },
    { DLMS_CLASS_DATA, 0, DLMS_OBIS_INIT(0, 0, 96, 1, 0, 255), 2, NULL, 0, NULL, data_read_fn, NULL, NULL, NULL },
    { DLMS_CLASS_DATA, 0, DLMS_OBIS_INIT(0, 0, 96, 1, 1, 255), 2, NULL, 0, NULL, data_read_fn, NULL, NULL, NULL },
    { DLMS_CLASS_DATA, 0, DLMS_OBIS_INIT(0, 0, 96, 1, 2, 255), 2, NULL, 0, NULL, data_read_fn, NULL, NULL, NULL },

    /* --- Class 3: Energy Registers (Wh / varh / VAh) --- */
    { DLMS_CLASS_REGISTER, 0, DLMS_OBIS_INIT(1, 0, 1, 8, 0, 255), 3, NULL, 0, NULL, register_read_fn, NULL, NULL, NULL },
    { DLMS_CLASS_REGISTER, 0, DLMS_OBIS_INIT(1, 0, 2, 8, 0, 255), 3, NULL, 0, NULL, register_read_fn, NULL, NULL, NULL },
    { DLMS_CLASS_REGISTER, 0, DLMS_OBIS_INIT(1, 0, 3, 8, 0, 255), 3, NULL, 0, NULL, register_read_fn, NULL, NULL, NULL },
    { DLMS_CLASS_REGISTER, 0, DLMS_OBIS_INIT(1, 0, 4, 8, 0, 255), 3, NULL, 0, NULL, register_read_fn, NULL, NULL, NULL },
    { DLMS_CLASS_REGISTER, 0, DLMS_OBIS_INIT(1, 0, 9, 8, 0, 255), 3, NULL, 0, NULL, register_read_fn, NULL, NULL, NULL },

    /* --- Class 3: Instantaneous Power Registers (W) --- */
    { DLMS_CLASS_REGISTER, 0, DLMS_OBIS_INIT(1, 0, 1, 7, 0, 255), 3, NULL, 0, NULL, register_read_fn, NULL, NULL, NULL },
    { DLMS_CLASS_REGISTER, 0, DLMS_OBIS_INIT(1, 0, 21, 7, 0, 255), 3, NULL, 0, NULL, register_read_fn, NULL, NULL, NULL },
    { DLMS_CLASS_REGISTER, 0, DLMS_OBIS_INIT(1, 0, 41, 7, 0, 255), 3, NULL, 0, NULL, register_read_fn, NULL, NULL, NULL },
    { DLMS_CLASS_REGISTER, 0, DLMS_OBIS_INIT(1, 0, 61, 7, 0, 255), 3, NULL, 0, NULL, register_read_fn, NULL, NULL, NULL },

    /* --- Class 3: RMS Voltages (Phase A, B, C) --- */
    { DLMS_CLASS_REGISTER, 0, DLMS_OBIS_INIT(1, 0, 32, 7, 0, 255), 3, NULL, 0, NULL, register_read_fn, NULL, NULL, NULL },
    { DLMS_CLASS_REGISTER, 0, DLMS_OBIS_INIT(1, 0, 52, 7, 0, 255), 3, NULL, 0, NULL, register_read_fn, NULL, NULL, NULL },
    { DLMS_CLASS_REGISTER, 0, DLMS_OBIS_INIT(1, 0, 72, 7, 0, 255), 3, NULL, 0, NULL, register_read_fn, NULL, NULL, NULL },

    /* --- Class 3: RMS Currents (Phase A, B, C) --- */
    { DLMS_CLASS_REGISTER, 0, DLMS_OBIS_INIT(1, 0, 31, 7, 0, 255), 3, NULL, 0, NULL, register_read_fn, NULL, NULL, NULL },
    { DLMS_CLASS_REGISTER, 0, DLMS_OBIS_INIT(1, 0, 51, 7, 0, 255), 3, NULL, 0, NULL, register_read_fn, NULL, NULL, NULL },
    { DLMS_CLASS_REGISTER, 0, DLMS_OBIS_INIT(1, 0, 71, 7, 0, 255), 3, NULL, 0, NULL, register_read_fn, NULL, NULL, NULL },

    /* --- Class 3: Frequency & Power Factor --- */
    { DLMS_CLASS_REGISTER, 0, DLMS_OBIS_INIT(1, 0, 14, 7, 0, 255), 3, NULL, 0, NULL, register_read_fn, NULL, NULL, NULL },
    { DLMS_CLASS_REGISTER, 0, DLMS_OBIS_INIT(1, 0, 13, 7, 0, 255), 3, NULL, 0, NULL, register_read_fn, NULL, NULL, NULL },

    /* --- Class 4: Maximum Demand Active Import --- */
    { DLMS_CLASS_EXTENDED_REGISTER, 0, DLMS_OBIS_INIT(1, 0, 1, 6, 0, 255), 5, NULL, 0, NULL, ext_register_read_fn, NULL, NULL, NULL },

    /* --- Class 8: Clock --- */
    { DLMS_CLASS_CLOCK, 0, DLMS_OBIS_INIT(0, 0, 1, 0, 0, 255), 4, NULL, 0, NULL, clock_read_fn, clock_write_fn, NULL, NULL },

    /* --- Class 15: Association LN Objects --- */
    { DLMS_CLASS_ASSOCIATION_LN, 0, DLMS_OBIS_INIT(0, 0, 40, 0, 0, 255), 4, NULL, 0, NULL, assoc_read_fn, NULL, NULL, NULL },
    { DLMS_CLASS_ASSOCIATION_LN, 0, DLMS_OBIS_INIT(0, 0, 40, 0, 1, 255), 4, NULL, 0, NULL, assoc_read_fn, NULL, NULL, NULL },
    { DLMS_CLASS_ASSOCIATION_LN, 0, DLMS_OBIS_INIT(0, 0, 40, 0, 3, 255), 4, NULL, 1, NULL, assoc_read_fn, NULL, assoc_method_fn, NULL },

    /* --- Class 64: Security Setup (Suite 0) --- */
    { DLMS_CLASS_SECURITY_SETUP, 0, DLMS_OBIS_INIT(0, 0, 43, 0, 1, 255), 5, NULL, 2, NULL, sec_setup_read_fn, NULL, sec_setup_method_fn, NULL },

    /* --- Class 7: Profile Generic (Load Survey) --- */
    { DLMS_CLASS_PROFILE_GENERIC, 0, DLMS_OBIS_INIT(1, 0, 99, 1, 0, 255), 8, NULL, 2, NULL, profile_read_fn, NULL, profile_method_fn, NULL },

    /* --- Class 70: Disconnect Control (Relay) --- */
    { DLMS_CLASS_DISCONNECT_CONTROL, 0, DLMS_OBIS_INIT(0, 0, 96, 3, 10, 255), 4, NULL, 2, NULL, disconnect_read_fn, NULL, disconnect_method_fn, NULL }
};

#define NUM_COSEM_OBJECTS (sizeof(s_cosem_objects) / sizeof(s_cosem_objects[0]))

static dlms_data_access_result_t encode_object_list(
    uint8_t *out_buf, uint16_t max_out_len, uint16_t *actual_len)
{
    dlms_axdr_encoder_t enc;
    dlms_axdr_encoder_init(&enc, out_buf, max_out_len);

    if (!dlms_axdr_encode_array_header(&enc, (uint16_t)NUM_COSEM_OBJECTS)) {
        return DLMS_RESULT_TEMPORARY_FAILURE;
    }

    for (size_t i = 0; i < NUM_COSEM_OBJECTS; i++) {
        const dlms_cosem_object_t *o = &s_cosem_objects[i];

        /* Structure of 4 elements: { class_id, version, logical_name, access_rights } */
        if (!dlms_axdr_encode_structure_header(&enc, 4)) return DLMS_RESULT_TEMPORARY_FAILURE;

        /* 1. class_id */
        if (!dlms_axdr_encode_u16(&enc, (uint16_t)o->class_id)) return DLMS_RESULT_TEMPORARY_FAILURE;

        /* 2. version */
        if (!dlms_axdr_encode_u8(&enc, o->version)) return DLMS_RESULT_TEMPORARY_FAILURE;

        /* 3. logical_name (6 bytes) */
        uint8_t obis_bytes[6] = { o->obis.a, o->obis.b, o->obis.c, o->obis.d, o->obis.e, o->obis.f };
        if (!dlms_axdr_encode_octet_string(&enc, obis_bytes, 6)) return DLMS_RESULT_TEMPORARY_FAILURE;

        /* 4. access_rights: structure(attribute_access, method_access) */
        if (!dlms_axdr_encode_structure_header(&enc, 2)) return DLMS_RESULT_TEMPORARY_FAILURE;

        /* 4a. attribute_access array */
        uint8_t num_attrs = o->num_attrs;
        if (num_attrs == 0) num_attrs = 2;
        if (!dlms_axdr_encode_array_header(&enc, num_attrs)) return DLMS_RESULT_TEMPORARY_FAILURE;

        for (uint8_t a = 1; a <= num_attrs; a++) {
            if (!dlms_axdr_encode_structure_header(&enc, 3)) return DLMS_RESULT_TEMPORARY_FAILURE;
            if (!dlms_axdr_encode_i8(&enc, (int8_t)a)) return DLMS_RESULT_TEMPORARY_FAILURE;

            /* Access mode: 1=read_only, 3=read_and_write */
            uint8_t mode = 1;
            if (o->class_id == DLMS_CLASS_CLOCK && a == 2) {
                mode = 3; /* Clock time can be written */
            } else if (o->class_id == DLMS_CLASS_PROFILE_GENERIC && (a == 3 || a == 5 || a == 6)) {
                mode = 0; /* Unimplemented optional profile generic attributes */
            }
            if (!dlms_axdr_encode_enum(&enc, mode)) return DLMS_RESULT_TEMPORARY_FAILURE;

            /* access_selectors: null-data */
            if (!dlms_axdr_encode_null(&enc)) return DLMS_RESULT_TEMPORARY_FAILURE;
        }

        /* 4b. method_access array */
        uint8_t num_methods = o->num_methods;
        if (!dlms_axdr_encode_array_header(&enc, num_methods)) return DLMS_RESULT_TEMPORARY_FAILURE;

        for (uint8_t m = 1; m <= num_methods; m++) {
            if (!dlms_axdr_encode_structure_header(&enc, 2)) return DLMS_RESULT_TEMPORARY_FAILURE;
            if (!dlms_axdr_encode_i8(&enc, (int8_t)m)) return DLMS_RESULT_TEMPORARY_FAILURE;
            if (!dlms_axdr_encode_bool(&enc, true)) return DLMS_RESULT_TEMPORARY_FAILURE;
        }
    }

    *actual_len = enc.offset;
    return DLMS_RESULT_SUCCESS;
}

void dlms_objects_init(void) {
    s_load_profile_count = 0;
    s_load_profile_head = 0;
    s_block_transfer.active = false;
}

dlms_cosem_object_t *dlms_find_object(uint16_t class_id, const dlms_obis_t *obis) {
    for (size_t i = 0; i < NUM_COSEM_OBJECTS; i++) {
        if (s_cosem_objects[i].class_id == class_id && DLMS_OBIS_EQUALS(s_cosem_objects[i].obis, *obis)) {
            return &s_cosem_objects[i];
        }
    }
    return NULL;
}

void dlms_profile_generic_capture(void) {
    const dlms_meter_adapter_t *ad = dlms_meter_adapter_get();
    dlms_load_profile_entry_t entry = {0};

    if (ad && ad->get_rtc_time) ad->get_rtc_time(&entry.timestamp);
    dlms_meter_energy_t energy = {0};
    if (ad && ad->get_energy) {
        ad->get_energy(&energy);
        entry.active_energy = energy.active_energy_import;
    }
    dlms_meter_instantaneous_t inst = {0};
    if (ad && ad->get_instantaneous) {
        ad->get_instantaneous(&inst);
        entry.voltage_a = inst.voltage_a;
        entry.voltage_b = inst.voltage_b;
        entry.voltage_c = inst.voltage_c;
        entry.current_a = inst.current_a;
        entry.current_b = inst.current_b;
        entry.current_c = inst.current_c;
    }

    s_load_profile_buffer[s_load_profile_head] = entry;
    s_load_profile_head = (s_load_profile_head + 1) % DLMS_LOAD_PROFILE_MAX_ENTRIES;
    if (s_load_profile_count < DLMS_LOAD_PROFILE_MAX_ENTRIES) {
        s_load_profile_count++;
    }
}

/* ========================================================================== */
/* xDLMS Services Dispatcher (GET, SET, ACTION)                              */
/* ========================================================================== */

static dlms_result_t handle_get_request(
    dlms_association_t *assoc,
    const uint8_t *req, uint16_t req_len,
    uint8_t *resp, uint16_t max_resp_len, uint16_t *resp_len)
{
    if (req_len < 3) return DLMS_ERR_INVALID_PARAM;

    uint8_t get_type = req[1];
    uint8_t invoke_id = req[2];

    if (get_type == DLMS_GET_REQUEST_NORMAL) {
        if (req_len < 12) return DLMS_ERR_FRAME_INCOMPLETE;

        uint16_t class_id = ((uint16_t)req[3] << 8) | req[4];
        dlms_obis_t obis = { req[5], req[6], req[7], req[8], req[9], req[10] };
        uint8_t attr_id = req[11];

        /* Check HLS association state */
        if (assoc->state == DLMS_ASSOC_STATE_PENDING_HLS) {
            /* Access denied until HLS verification is complete */
            resp[0] = DLMS_TAG_GET_RESPONSE;
            resp[1] = DLMS_GET_RESPONSE_NORMAL;
            resp[2] = invoke_id;
            resp[3] = 0x01; /* Result: Data-access-result */
            resp[4] = (uint8_t)DLMS_RESULT_READ_WRITE_DENIED;
            *resp_len = 5;
            return DLMS_OK;
        }

        dlms_cosem_object_t *obj = dlms_find_object(class_id, &obis);
        if (!obj || !obj->read_fn) {
            resp[0] = DLMS_TAG_GET_RESPONSE;
            resp[1] = DLMS_GET_RESPONSE_NORMAL;
            resp[2] = invoke_id;
            resp[3] = 0x01; /* Result: Data-access-result */
            resp[4] = (uint8_t)DLMS_RESULT_OBJECT_UNDEFINED;
            *resp_len = 5;
            return DLMS_OK;
        }

        static uint8_t s_get_data_buf[DLMS_MAX_PDU_SIZE * 2];
        uint16_t actual_data_len = 0;
        dlms_data_access_result_t res = obj->read_fn(obj, attr_id, s_get_data_buf, sizeof(s_get_data_buf), &actual_data_len, assoc);

        if (res != DLMS_RESULT_SUCCESS) {
            resp[0] = DLMS_TAG_GET_RESPONSE;
            resp[1] = DLMS_GET_RESPONSE_NORMAL;
            resp[2] = invoke_id;
            resp[3] = 0x01; /* Result: Data-access-result */
            resp[4] = (uint8_t)res;
            *resp_len = 5;
            return DLMS_OK;
        }

        /* Check if Block Transfer is required */
        if (actual_data_len > DLMS_BLOCK_TRANSFER_SIZE) {
            s_block_transfer.active = true;
            s_block_transfer.current_block_num = 1;
            s_block_transfer.cache_len = actual_data_len;
            s_block_transfer.sent_offset = DLMS_BLOCK_TRANSFER_SIZE;
            memcpy(s_block_transfer.cache_buf, s_get_data_buf, actual_data_len);

            resp[0] = DLMS_TAG_GET_RESPONSE;
            resp[1] = DLMS_GET_RESPONSE_DATABLOCK;
            resp[2] = invoke_id;
            resp[3] = 0x00; /* last-block = false */
            resp[4] = 0x00; resp[5] = 0x00; resp[6] = 0x00; resp[7] = 0x01; /* block-number = 1 */
            resp[8] = 0x00; /* raw-data */

            dlms_axdr_encoder_t enc;
            dlms_axdr_encoder_init(&enc, resp + 9, max_resp_len - 9);
            dlms_axdr_encode_length(&enc, DLMS_BLOCK_TRANSFER_SIZE);
            dlms_axdr_encode_raw(&enc, s_get_data_buf, DLMS_BLOCK_TRANSFER_SIZE);
            *resp_len = 9 + enc.offset;
            return DLMS_OK;
        }

        /* Normal GET Response */
        resp[0] = DLMS_TAG_GET_RESPONSE;
        resp[1] = DLMS_GET_RESPONSE_NORMAL;
        resp[2] = invoke_id;
        resp[3] = 0x00; /* Result: Data */
        memcpy(resp + 4, s_get_data_buf, actual_data_len);
        *resp_len = 4 + actual_data_len;
        return DLMS_OK;
    } else if (get_type == DLMS_GET_REQUEST_NEXT) {
        /* Block Transfer GET-Request-Next */
        if (req_len < 7) return DLMS_ERR_INVALID_PARAM;
        if (!s_block_transfer.active) {
            resp[0] = DLMS_TAG_GET_RESPONSE;
            resp[1] = DLMS_GET_RESPONSE_DATABLOCK;
            resp[2] = invoke_id;
            resp[3] = 0x01; /* last-block = true */
            resp[4] = 0x00; resp[5] = 0x00; resp[6] = 0x00; resp[7] = 0x00;
            resp[8] = 0x01; /* result: [1] IMPLICIT Data-Access-Result */
            resp[9] = (uint8_t)DLMS_RESULT_NO_LONG_GET_IN_PROGRESS;
            *resp_len = 10;
            return DLMS_OK;
        }

        uint32_t ack_block_num = ((uint32_t)req[3] << 24) | ((uint32_t)req[4] << 16) |
                                 ((uint32_t)req[5] << 8) | req[6];
        uint32_t next_block_num = ack_block_num + 1;

        uint16_t offset = (uint16_t)(ack_block_num * DLMS_BLOCK_TRANSFER_SIZE);
        if (offset >= s_block_transfer.cache_len) {
            s_block_transfer.active = false;
            resp[0] = DLMS_TAG_GET_RESPONSE;
            resp[1] = DLMS_GET_RESPONSE_DATABLOCK;
            resp[2] = invoke_id;
            resp[3] = 0x01; /* last-block = true */
            resp[4] = (uint8_t)(next_block_num >> 24);
            resp[5] = (uint8_t)(next_block_num >> 16);
            resp[6] = (uint8_t)(next_block_num >> 8);
            resp[7] = (uint8_t)(next_block_num & 0xFF);
            resp[8] = 0x01; /* result: [1] IMPLICIT Data-Access-Result */
            resp[9] = (uint8_t)DLMS_RESULT_DATA_BLOCK_NUMBER_INV;
            *resp_len = 10;
            return DLMS_OK;
        }

        uint16_t rem = s_block_transfer.cache_len - offset;
        uint16_t send_len = (rem > DLMS_BLOCK_TRANSFER_SIZE) ? DLMS_BLOCK_TRANSFER_SIZE : rem;
        bool last_block = (offset + send_len >= s_block_transfer.cache_len);

        resp[0] = DLMS_TAG_GET_RESPONSE;
        resp[1] = DLMS_GET_RESPONSE_DATABLOCK;
        resp[2] = invoke_id;
        resp[3] = last_block ? 0x01 : 0x00;
        resp[4] = (uint8_t)(next_block_num >> 24);
        resp[5] = (uint8_t)(next_block_num >> 16);
        resp[6] = (uint8_t)(next_block_num >> 8);
        resp[7] = (uint8_t)(next_block_num & 0xFF);
        resp[8] = 0x00; /* raw-data */

        dlms_axdr_encoder_t enc;
        dlms_axdr_encoder_init(&enc, resp + 9, max_resp_len - 9);
        dlms_axdr_encode_length(&enc, send_len);
        dlms_axdr_encode_raw(&enc, s_block_transfer.cache_buf + offset, send_len);
        *resp_len = 9 + enc.offset;

        s_block_transfer.current_block_num = next_block_num;
        s_block_transfer.sent_offset = offset + send_len;

        if (last_block) {
            s_block_transfer.active = false;
        }
        return DLMS_OK;
    }

    return DLMS_ERR_NOT_SUPPORTED;
}

static dlms_result_t handle_set_request(
    dlms_association_t *assoc,
    const uint8_t *req, uint16_t req_len,
    uint8_t *resp, uint16_t max_resp_len, uint16_t *resp_len)
{
    (void)max_resp_len;
    if (req_len < 12) return DLMS_ERR_FRAME_INCOMPLETE;

    uint8_t invoke_id = req[2];
    uint16_t class_id = ((uint16_t)req[3] << 8) | req[4];
    dlms_obis_t obis = { req[5], req[6], req[7], req[8], req[9], req[10] };
    uint8_t attr_id = req[11];
    const uint8_t *data = req + 12;
    uint16_t data_len = req_len - 12;

    dlms_cosem_object_t *obj = dlms_find_object(class_id, &obis);
    dlms_data_access_result_t res = DLMS_RESULT_OBJECT_UNDEFINED;

    if (obj && obj->write_fn) {
        res = obj->write_fn(obj, attr_id, data, data_len, assoc);
    } else if (obj) {
        res = DLMS_RESULT_READ_WRITE_DENIED;
    }

    resp[0] = DLMS_TAG_SET_RESPONSE;
    resp[1] = DLMS_SET_REQUEST_NORMAL;
    resp[2] = invoke_id;
    resp[3] = (uint8_t)res;
    *resp_len = 4;
    return DLMS_OK;
}

static dlms_result_t handle_action_request(
    dlms_association_t *assoc,
    const uint8_t *req, uint16_t req_len,
    uint8_t *resp, uint16_t max_resp_len, uint16_t *resp_len)
{
    (void)max_resp_len;
    if (req_len < 12) return DLMS_ERR_FRAME_INCOMPLETE;

    uint8_t invoke_id = req[2];
    uint16_t class_id = ((uint16_t)req[3] << 8) | req[4];
    dlms_obis_t obis = { req[5], req[6], req[7], req[8], req[9], req[10] };
    uint8_t method_id = req[11];
    const uint8_t *in_data = (req_len > 12) ? req + 12 : NULL;
    uint16_t in_len = (req_len > 12) ? req_len - 12 : 0;

    dlms_cosem_object_t *obj = dlms_find_object(class_id, &obis);
    dlms_action_result_t act_res = DLMS_ACTION_RESULT_OBJECT_UNDEFINED;
    uint8_t out_data[DLMS_MAX_PDU_SIZE];
    uint16_t out_len = 0;

    if (obj && obj->method_fn) {
        act_res = obj->method_fn(obj, method_id, in_data, in_len, out_data, sizeof(out_data), &out_len, assoc);
    }

    resp[0] = DLMS_TAG_ACTION_RESPONSE;
    resp[1] = DLMS_ACTION_REQUEST_NORMAL;
    resp[2] = invoke_id;
    resp[3] = (uint8_t)act_res;

    if (out_len > 0 && act_res == DLMS_ACTION_RESULT_SUCCESS) {
        resp[4] = 0x01; /* data-present = true */
        memcpy(resp + 5, out_data, out_len);
        *resp_len = 5 + out_len;
    } else {
        resp[4] = 0x00; /* data-present = false */
        *resp_len = 5;
    }

    return DLMS_OK;
}

dlms_result_t dlms_process_request(
    dlms_association_t *assoc,
    const uint8_t *req_apdu, uint16_t req_len,
    uint8_t *resp_apdu, uint16_t max_resp_len, uint16_t *resp_len)
{
    if (!assoc || !req_apdu || req_len < 1 || !resp_apdu || !resp_len) return DLMS_ERR_INVALID_PARAM;

    uint8_t tag = req_apdu[0];

    if (tag == DLMS_TAG_GET_REQUEST) {
        return handle_get_request(assoc, req_apdu, req_len, resp_apdu, max_resp_len, resp_len);
    } else if (tag == DLMS_TAG_SET_REQUEST) {
        return handle_set_request(assoc, req_apdu, req_len, resp_apdu, max_resp_len, resp_len);
    } else if (tag == DLMS_TAG_ACTION_REQUEST) {
        return handle_action_request(assoc, req_apdu, req_len, resp_apdu, max_resp_len, resp_len);
    }

    return DLMS_ERR_NOT_SUPPORTED;
}
