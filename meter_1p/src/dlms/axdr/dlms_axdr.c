/**
 * @file dlms_axdr.c
 * @brief A-XDR Encoding and Decoding Implementation
 */

#include "dlms_axdr.h"
#include <string.h>

void dlms_axdr_encoder_init(dlms_axdr_encoder_t *enc, uint8_t *buf, uint16_t capacity) {
    enc->p_buf = buf;
    enc->capacity = capacity;
    enc->offset = 0;
}

void dlms_axdr_decoder_init(dlms_axdr_decoder_t *dec, const uint8_t *buf, uint16_t length) {
    dec->p_buf = buf;
    dec->length = length;
    dec->offset = 0;
}

static inline bool enc_has_space(const dlms_axdr_encoder_t *enc, uint16_t need) {
    return (uint32_t)enc->offset + need <= enc->capacity;
}

static inline bool dec_has_data(const dlms_axdr_decoder_t *dec, uint16_t need) {
    return (uint32_t)dec->offset + need <= dec->length;
}

bool dlms_axdr_encode_length(dlms_axdr_encoder_t *enc, uint32_t len) {
    if (len <= 0x7F) {
        if (!enc_has_space(enc, 1)) return false;
        enc->p_buf[enc->offset++] = (uint8_t)len;
    } else if (len <= 0xFF) {
        if (!enc_has_space(enc, 2)) return false;
        enc->p_buf[enc->offset++] = 0x81;
        enc->p_buf[enc->offset++] = (uint8_t)len;
    } else if (len <= 0xFFFF) {
        if (!enc_has_space(enc, 3)) return false;
        enc->p_buf[enc->offset++] = 0x82;
        enc->p_buf[enc->offset++] = (uint8_t)(len >> 8);
        enc->p_buf[enc->offset++] = (uint8_t)(len & 0xFF);
    } else {
        if (!enc_has_space(enc, 5)) return false;
        enc->p_buf[enc->offset++] = 0x84;
        enc->p_buf[enc->offset++] = (uint8_t)(len >> 24);
        enc->p_buf[enc->offset++] = (uint8_t)(len >> 16);
        enc->p_buf[enc->offset++] = (uint8_t)(len >> 8);
        enc->p_buf[enc->offset++] = (uint8_t)(len & 0xFF);
    }
    return true;
}

bool dlms_axdr_decode_length(dlms_axdr_decoder_t *dec, uint32_t *len) {
    if (!dec_has_data(dec, 1)) return false;
    uint8_t b = dec->p_buf[dec->offset++];
    if ((b & 0x80) == 0) {
        *len = b;
        return true;
    }
    uint8_t num_bytes = b & 0x7F;
    if (num_bytes == 0 || num_bytes > 4 || !dec_has_data(dec, num_bytes)) return false;

    uint32_t val = 0;
    for (uint8_t i = 0; i < num_bytes; i++) {
        val = (val << 8) | dec->p_buf[dec->offset++];
    }
    *len = val;
    return true;
}

bool dlms_axdr_encode_null(dlms_axdr_encoder_t *enc) {
    if (!enc_has_space(enc, 1)) return false;
    enc->p_buf[enc->offset++] = DLMS_DATATYPE_NULL;
    return true;
}

bool dlms_axdr_encode_bool(dlms_axdr_encoder_t *enc, bool val) {
    if (!enc_has_space(enc, 2)) return false;
    enc->p_buf[enc->offset++] = DLMS_DATATYPE_BOOLEAN;
    enc->p_buf[enc->offset++] = val ? 1 : 0;
    return true;
}

bool dlms_axdr_encode_u8(dlms_axdr_encoder_t *enc, uint8_t val) {
    if (!enc_has_space(enc, 2)) return false;
    enc->p_buf[enc->offset++] = DLMS_DATATYPE_UNSIGNED;
    enc->p_buf[enc->offset++] = val;
    return true;
}

bool dlms_axdr_encode_i8(dlms_axdr_encoder_t *enc, int8_t val) {
    if (!enc_has_space(enc, 2)) return false;
    enc->p_buf[enc->offset++] = DLMS_DATATYPE_INTEGER;
    enc->p_buf[enc->offset++] = (uint8_t)val;
    return true;
}

bool dlms_axdr_encode_u16(dlms_axdr_encoder_t *enc, uint16_t val) {
    if (!enc_has_space(enc, 3)) return false;
    enc->p_buf[enc->offset++] = DLMS_DATATYPE_LONG_UNSIGNED;
    enc->p_buf[enc->offset++] = (uint8_t)(val >> 8);
    enc->p_buf[enc->offset++] = (uint8_t)(val & 0xFF);
    return true;
}

bool dlms_axdr_encode_i16(dlms_axdr_encoder_t *enc, int16_t val) {
    if (!enc_has_space(enc, 3)) return false;
    enc->p_buf[enc->offset++] = DLMS_DATATYPE_LONG;
    enc->p_buf[enc->offset++] = (uint8_t)((uint16_t)val >> 8);
    enc->p_buf[enc->offset++] = (uint8_t)((uint16_t)val & 0xFF);
    return true;
}

bool dlms_axdr_encode_u32(dlms_axdr_encoder_t *enc, uint32_t val) {
    if (!enc_has_space(enc, 5)) return false;
    enc->p_buf[enc->offset++] = DLMS_DATATYPE_DOUBLE_LONG_UNSIGNED;
    enc->p_buf[enc->offset++] = (uint8_t)(val >> 24);
    enc->p_buf[enc->offset++] = (uint8_t)(val >> 16);
    enc->p_buf[enc->offset++] = (uint8_t)(val >> 8);
    enc->p_buf[enc->offset++] = (uint8_t)(val & 0xFF);
    return true;
}

bool dlms_axdr_encode_i32(dlms_axdr_encoder_t *enc, int32_t val) {
    if (!enc_has_space(enc, 5)) return false;
    enc->p_buf[enc->offset++] = DLMS_DATATYPE_DOUBLE_LONG;
    enc->p_buf[enc->offset++] = (uint8_t)((uint32_t)val >> 24);
    enc->p_buf[enc->offset++] = (uint8_t)((uint32_t)val >> 16);
    enc->p_buf[enc->offset++] = (uint8_t)((uint32_t)val >> 8);
    enc->p_buf[enc->offset++] = (uint8_t)((uint32_t)val & 0xFF);
    return true;
}

bool dlms_axdr_encode_u64(dlms_axdr_encoder_t *enc, uint64_t val) {
    if (!enc_has_space(enc, 9)) return false;
    enc->p_buf[enc->offset++] = DLMS_DATATYPE_LONG64_UNSIGNED;
    for (int i = 7; i >= 0; i--) {
        enc->p_buf[enc->offset++] = (uint8_t)(val >> (i * 8));
    }
    return true;
}

bool dlms_axdr_encode_i64(dlms_axdr_encoder_t *enc, int64_t val) {
    if (!enc_has_space(enc, 9)) return false;
    enc->p_buf[enc->offset++] = DLMS_DATATYPE_LONG64;
    for (int i = 7; i >= 0; i--) {
        enc->p_buf[enc->offset++] = (uint8_t)((uint64_t)val >> (i * 8));
    }
    return true;
}

bool dlms_axdr_encode_float(dlms_axdr_encoder_t *enc, float val) {
    if (!enc_has_space(enc, 5)) return false;
    enc->p_buf[enc->offset++] = DLMS_DATATYPE_FLOAT32;
    uint32_t raw;
    memcpy(&raw, &val, 4);
    enc->p_buf[enc->offset++] = (uint8_t)(raw >> 24);
    enc->p_buf[enc->offset++] = (uint8_t)(raw >> 16);
    enc->p_buf[enc->offset++] = (uint8_t)(raw >> 8);
    enc->p_buf[enc->offset++] = (uint8_t)(raw & 0xFF);
    return true;
}

bool dlms_axdr_encode_enum(dlms_axdr_encoder_t *enc, uint8_t val) {
    if (!enc_has_space(enc, 2)) return false;
    enc->p_buf[enc->offset++] = DLMS_DATATYPE_ENUM;
    enc->p_buf[enc->offset++] = val;
    return true;
}

bool dlms_axdr_encode_octet_string(dlms_axdr_encoder_t *enc, const uint8_t *data, uint16_t len) {
    if (!enc_has_space(enc, 1)) return false;
    enc->p_buf[enc->offset++] = DLMS_DATATYPE_OCTET_STRING;
    if (!dlms_axdr_encode_length(enc, len)) return false;
    if (!enc_has_space(enc, len)) return false;
    if (len > 0 && data != NULL) {
        memcpy(enc->p_buf + enc->offset, data, len);
        enc->offset += len;
    }
    return true;
}

bool dlms_axdr_encode_visible_string(dlms_axdr_encoder_t *enc, const char *str) {
    uint16_t len = (uint16_t)strlen(str);
    if (!enc_has_space(enc, 1)) return false;
    enc->p_buf[enc->offset++] = DLMS_DATATYPE_VISIBLE_STRING;
    if (!dlms_axdr_encode_length(enc, len)) return false;
    if (!enc_has_space(enc, len)) return false;
    memcpy(enc->p_buf + enc->offset, str, len);
    enc->offset += len;
    return true;
}

bool dlms_axdr_encode_bit_string(dlms_axdr_encoder_t *enc, const uint8_t *bits, uint16_t num_bits) {
    uint16_t num_bytes = (num_bits + 7) / 8;
    if (!enc_has_space(enc, 1)) return false;
    enc->p_buf[enc->offset++] = DLMS_DATATYPE_BIT_STRING;
    if (!dlms_axdr_encode_length(enc, num_bits)) return false;
    if (!enc_has_space(enc, num_bytes)) return false;
    memcpy(enc->p_buf + enc->offset, bits, num_bytes);
    enc->offset += num_bytes;
    return true;
}

bool dlms_axdr_encode_structure_header(dlms_axdr_encoder_t *enc, uint16_t num_elements) {
    if (!enc_has_space(enc, 1)) return false;
    enc->p_buf[enc->offset++] = DLMS_DATATYPE_STRUCTURE;
    return dlms_axdr_encode_length(enc, num_elements);
}

bool dlms_axdr_encode_array_header(dlms_axdr_encoder_t *enc, uint16_t num_elements) {
    if (!enc_has_space(enc, 1)) return false;
    enc->p_buf[enc->offset++] = DLMS_DATATYPE_ARRAY;
    return dlms_axdr_encode_length(enc, num_elements);
}

bool dlms_axdr_encode_date_time(dlms_axdr_encoder_t *enc, const dlms_date_time_t *dt) {
    uint8_t dt_raw[12];
    dt_raw[0] = (uint8_t)(dt->year >> 8);
    dt_raw[1] = (uint8_t)(dt->year & 0xFF);
    dt_raw[2] = dt->month;
    dt_raw[3] = dt->day_of_month;
    dt_raw[4] = dt->day_of_week;
    dt_raw[5] = dt->hour;
    dt_raw[6] = dt->minute;
    dt_raw[7] = dt->second;
    dt_raw[8] = dt->hundredths;
    dt_raw[9] = (uint8_t)((uint16_t)dt->deviation >> 8);
    dt_raw[10] = (uint8_t)((uint16_t)dt->deviation & 0xFF);
    dt_raw[11] = dt->clock_status;

    return dlms_axdr_encode_octet_string(enc, dt_raw, 12);
}

bool dlms_axdr_encode_scaler_unit(dlms_axdr_encoder_t *enc, int8_t scaler, dlms_unit_t unit) {
    if (!dlms_axdr_encode_structure_header(enc, 2)) return false;
    if (!dlms_axdr_encode_i8(enc, scaler)) return false;
    if (!dlms_axdr_encode_enum(enc, (uint8_t)unit)) return false;
    return true;
}

bool dlms_axdr_encode_raw(dlms_axdr_encoder_t *enc, const uint8_t *data, uint16_t len) {
    if (!enc_has_space(enc, len)) return false;
    memcpy(enc->p_buf + enc->offset, data, len);
    enc->offset += len;
    return true;
}

bool dlms_axdr_decode_tag(dlms_axdr_decoder_t *dec, dlms_datatype_t *tag) {
    if (!dec_has_data(dec, 1)) return false;
    *tag = (dlms_datatype_t)dec->p_buf[dec->offset++];
    return true;
}

bool dlms_axdr_decode_bool(dlms_axdr_decoder_t *dec, bool *val) {
    dlms_datatype_t tag;
    if (!dlms_axdr_decode_tag(dec, &tag) || tag != DLMS_DATATYPE_BOOLEAN) return false;
    if (!dec_has_data(dec, 1)) return false;
    *val = (dec->p_buf[dec->offset++] != 0);
    return true;
}

bool dlms_axdr_decode_u8(dlms_axdr_decoder_t *dec, uint8_t *val) {
    dlms_datatype_t tag;
    if (!dlms_axdr_decode_tag(dec, &tag) || tag != DLMS_DATATYPE_UNSIGNED) return false;
    if (!dec_has_data(dec, 1)) return false;
    *val = dec->p_buf[dec->offset++];
    return true;
}

bool dlms_axdr_decode_i8(dlms_axdr_decoder_t *dec, int8_t *val) {
    dlms_datatype_t tag;
    if (!dlms_axdr_decode_tag(dec, &tag) || tag != DLMS_DATATYPE_INTEGER) return false;
    if (!dec_has_data(dec, 1)) return false;
    *val = (int8_t)dec->p_buf[dec->offset++];
    return true;
}

bool dlms_axdr_decode_u16(dlms_axdr_decoder_t *dec, uint16_t *val) {
    dlms_datatype_t tag;
    if (!dlms_axdr_decode_tag(dec, &tag) || tag != DLMS_DATATYPE_LONG_UNSIGNED) return false;
    if (!dec_has_data(dec, 2)) return false;
    *val = ((uint16_t)dec->p_buf[dec->offset] << 8) | dec->p_buf[dec->offset + 1];
    dec->offset += 2;
    return true;
}

bool dlms_axdr_decode_i16(dlms_axdr_decoder_t *dec, int16_t *val) {
    dlms_datatype_t tag;
    if (!dlms_axdr_decode_tag(dec, &tag) || tag != DLMS_DATATYPE_LONG) return false;
    if (!dec_has_data(dec, 2)) return false;
    *val = (int16_t)(((uint16_t)dec->p_buf[dec->offset] << 8) | dec->p_buf[dec->offset + 1]);
    dec->offset += 2;
    return true;
}

bool dlms_axdr_decode_u32(dlms_axdr_decoder_t *dec, uint32_t *val) {
    dlms_datatype_t tag;
    if (!dlms_axdr_decode_tag(dec, &tag) || tag != DLMS_DATATYPE_DOUBLE_LONG_UNSIGNED) return false;
    if (!dec_has_data(dec, 4)) return false;
    *val = ((uint32_t)dec->p_buf[dec->offset] << 24) |
           ((uint32_t)dec->p_buf[dec->offset + 1] << 16) |
           ((uint32_t)dec->p_buf[dec->offset + 2] << 8) |
            (uint32_t)dec->p_buf[dec->offset + 3];
    dec->offset += 4;
    return true;
}

bool dlms_axdr_decode_i32(dlms_axdr_decoder_t *dec, int32_t *val) {
    dlms_datatype_t tag;
    if (!dlms_axdr_decode_tag(dec, &tag) || tag != DLMS_DATATYPE_DOUBLE_LONG) return false;
    if (!dec_has_data(dec, 4)) return false;
    *val = (int32_t)(((uint32_t)dec->p_buf[dec->offset] << 24) |
                     ((uint32_t)dec->p_buf[dec->offset + 1] << 16) |
                     ((uint32_t)dec->p_buf[dec->offset + 2] << 8) |
                      (uint32_t)dec->p_buf[dec->offset + 3]);
    dec->offset += 4;
    return true;
}

bool dlms_axdr_decode_u64(dlms_axdr_decoder_t *dec, uint64_t *val) {
    dlms_datatype_t tag;
    if (!dlms_axdr_decode_tag(dec, &tag) || tag != DLMS_DATATYPE_LONG64_UNSIGNED) return false;
    if (!dec_has_data(dec, 8)) return false;
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) {
        v = (v << 8) | dec->p_buf[dec->offset++];
    }
    *val = v;
    return true;
}

bool dlms_axdr_decode_enum(dlms_axdr_decoder_t *dec, uint8_t *val) {
    dlms_datatype_t tag;
    if (!dlms_axdr_decode_tag(dec, &tag) || tag != DLMS_DATATYPE_ENUM) return false;
    if (!dec_has_data(dec, 1)) return false;
    *val = dec->p_buf[dec->offset++];
    return true;
}

bool dlms_axdr_decode_octet_string(dlms_axdr_decoder_t *dec, const uint8_t **data, uint32_t *len) {
    dlms_datatype_t tag;
    if (!dlms_axdr_decode_tag(dec, &tag) || tag != DLMS_DATATYPE_OCTET_STRING) return false;
    if (!dlms_axdr_decode_length(dec, len)) return false;
    if (!dec_has_data(dec, (uint16_t)*len)) return false;
    *data = dec->p_buf + dec->offset;
    dec->offset += (uint16_t)*len;
    return true;
}

bool dlms_axdr_decode_visible_string(dlms_axdr_decoder_t *dec, const char **str, uint32_t *len) {
    dlms_datatype_t tag;
    if (!dlms_axdr_decode_tag(dec, &tag) || tag != DLMS_DATATYPE_VISIBLE_STRING) return false;
    if (!dlms_axdr_decode_length(dec, len)) return false;
    if (!dec_has_data(dec, (uint16_t)*len)) return false;
    *str = (const char *)(dec->p_buf + dec->offset);
    dec->offset += (uint16_t)*len;
    return true;
}

bool dlms_axdr_decode_structure_header(dlms_axdr_decoder_t *dec, uint32_t *num_elements) {
    dlms_datatype_t tag;
    if (!dlms_axdr_decode_tag(dec, &tag) || tag != DLMS_DATATYPE_STRUCTURE) return false;
    return dlms_axdr_decode_length(dec, num_elements);
}

bool dlms_axdr_decode_array_header(dlms_axdr_decoder_t *dec, uint32_t *num_elements) {
    dlms_datatype_t tag;
    if (!dlms_axdr_decode_tag(dec, &tag) || tag != DLMS_DATATYPE_ARRAY) return false;
    return dlms_axdr_decode_length(dec, num_elements);
}

bool dlms_axdr_decode_date_time(dlms_axdr_decoder_t *dec, dlms_date_time_t *dt) {
    const uint8_t *raw;
    uint32_t len;
    if (!dlms_axdr_decode_octet_string(dec, &raw, &len) || len != 12) return false;

    dt->year = ((uint16_t)raw[0] << 8) | raw[1];
    dt->month = raw[2];
    dt->day_of_month = raw[3];
    dt->day_of_week = raw[4];
    dt->hour = raw[5];
    dt->minute = raw[6];
    dt->second = raw[7];
    dt->hundredths = raw[8];
    dt->deviation = (int16_t)(((uint16_t)raw[9] << 8) | raw[10]);
    dt->clock_status = raw[11];
    return true;
}
