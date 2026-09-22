/**
 * @file dlms_axdr.h
 * @brief A-XDR Encoding and Decoding Engine (IEC 62056-5-3)
 */

#ifndef DLMS_AXDR_H
#define DLMS_AXDR_H

#include "dlms_types.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A-XDR Encoder Buffer Context */
typedef struct {
    uint8_t *p_buf;
    uint16_t capacity;
    uint16_t offset;
} dlms_axdr_encoder_t;

/* A-XDR Decoder Buffer Context */
typedef struct {
    const uint8_t *p_buf;
    uint16_t length;
    uint16_t offset;
} dlms_axdr_decoder_t;

/* Encoder / Decoder Context Initialization */
void dlms_axdr_encoder_init(dlms_axdr_encoder_t *enc, uint8_t *buf, uint16_t capacity);
void dlms_axdr_decoder_init(dlms_axdr_decoder_t *dec, const uint8_t *buf, uint16_t length);

/* BER Length Encoding / Decoding */
bool dlms_axdr_encode_length(dlms_axdr_encoder_t *enc, uint32_t len);
bool dlms_axdr_decode_length(dlms_axdr_decoder_t *dec, uint32_t *len);

/* Primitive Encoders */
bool dlms_axdr_encode_null(dlms_axdr_encoder_t *enc);
bool dlms_axdr_encode_bool(dlms_axdr_encoder_t *enc, bool val);
bool dlms_axdr_encode_u8(dlms_axdr_encoder_t *enc, uint8_t val);
bool dlms_axdr_encode_i8(dlms_axdr_encoder_t *enc, int8_t val);
bool dlms_axdr_encode_u16(dlms_axdr_encoder_t *enc, uint16_t val);
bool dlms_axdr_encode_i16(dlms_axdr_encoder_t *enc, int16_t val);
bool dlms_axdr_encode_u32(dlms_axdr_encoder_t *enc, uint32_t val);
bool dlms_axdr_encode_i32(dlms_axdr_encoder_t *enc, int32_t val);
bool dlms_axdr_encode_u64(dlms_axdr_encoder_t *enc, uint64_t val);
bool dlms_axdr_encode_i64(dlms_axdr_encoder_t *enc, int64_t val);
bool dlms_axdr_encode_float(dlms_axdr_encoder_t *enc, float val);
bool dlms_axdr_encode_enum(dlms_axdr_encoder_t *enc, uint8_t val);

/* Complex Type Encoders */
bool dlms_axdr_encode_octet_string(dlms_axdr_encoder_t *enc, const uint8_t *data, uint16_t len);
bool dlms_axdr_encode_visible_string(dlms_axdr_encoder_t *enc, const char *str);
bool dlms_axdr_encode_bit_string(dlms_axdr_encoder_t *enc, const uint8_t *bits, uint16_t num_bits);
bool dlms_axdr_encode_structure_header(dlms_axdr_encoder_t *enc, uint16_t num_elements);
bool dlms_axdr_encode_array_header(dlms_axdr_encoder_t *enc, uint16_t num_elements);
bool dlms_axdr_encode_date_time(dlms_axdr_encoder_t *enc, const dlms_date_time_t *dt);
bool dlms_axdr_encode_scaler_unit(dlms_axdr_encoder_t *enc, int8_t scaler, dlms_unit_t unit);

/* Primitive Decoders */
bool dlms_axdr_decode_tag(dlms_axdr_decoder_t *dec, dlms_datatype_t *tag);
bool dlms_axdr_decode_bool(dlms_axdr_decoder_t *dec, bool *val);
bool dlms_axdr_decode_u8(dlms_axdr_decoder_t *dec, uint8_t *val);
bool dlms_axdr_decode_i8(dlms_axdr_decoder_t *dec, int8_t *val);
bool dlms_axdr_decode_u16(dlms_axdr_decoder_t *dec, uint16_t *val);
bool dlms_axdr_decode_i16(dlms_axdr_decoder_t *dec, int16_t *val);
bool dlms_axdr_decode_u32(dlms_axdr_decoder_t *dec, uint32_t *val);
bool dlms_axdr_decode_i32(dlms_axdr_decoder_t *dec, int32_t *val);
bool dlms_axdr_decode_u64(dlms_axdr_decoder_t *dec, uint64_t *val);
bool dlms_axdr_decode_enum(dlms_axdr_decoder_t *dec, uint8_t *val);

/* Complex Type Decoders */
bool dlms_axdr_decode_octet_string(dlms_axdr_decoder_t *dec, const uint8_t **data, uint32_t *len);
bool dlms_axdr_decode_visible_string(dlms_axdr_decoder_t *dec, const char **str, uint32_t *len);
bool dlms_axdr_decode_structure_header(dlms_axdr_decoder_t *dec, uint32_t *num_elements);
bool dlms_axdr_decode_array_header(dlms_axdr_decoder_t *dec, uint32_t *num_elements);
bool dlms_axdr_decode_date_time(dlms_axdr_decoder_t *dec, dlms_date_time_t *dt);

/* Raw byte appending helper */
bool dlms_axdr_encode_raw(dlms_axdr_encoder_t *enc, const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_AXDR_H */
