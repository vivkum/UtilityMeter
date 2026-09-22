/**
 * @file dlms_security_suite0.h
 * @brief DLMS Security Suite 0 Engine: AES-GCM-128, HLS 5 GMAC, and Ciphered APDUs
 * 
 * Standards: IEC 62056-5-3, DLMS Green Book Ed 9/10
 */

#ifndef DLMS_SECURITY_SUITE0_H
#define DLMS_SECURITY_SUITE0_H

#include "dlms_types.h"
#include "dlms_security.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Decrypt and authenticate a ciphered APDU (GLO-* or GENERAL-GLO-CIPHERING).
 * @param sec Security context.
 * @param in_apdu Raw ciphered APDU buffer.
 * @param in_len Length of ciphered APDU.
 * @param out_plain Output buffer for decrypted plaintext APDU.
 * @param max_plain_len Capacity of out_plain.
 * @param plain_len Output actual length of decrypted APDU.
 * @return DLMS_OK on success, error otherwise.
 */
dlms_result_t dlms_suite0_decrypt_apdu(
    dlms_security_context_t *sec,
    const uint8_t *in_apdu, uint16_t in_len,
    uint8_t *out_plain, uint16_t max_plain_len, uint16_t *plain_len);

/**
 * @brief Encrypt and authenticate a plaintext APDU into a ciphered APDU.
 * @param sec Security context.
 * @param sc Security control byte (e.g. DLMS_SC_SUITE0_AUTH_ENC = 0x30).
 * @param cipher_tag Desired APDU tag (e.g. DLMS_TAG_GLO_GET_RESPONSE = 0xCC, DLMS_TAG_GENERAL_GLO_CIPHERING = 0xDB).
 * @param plain_apdu Plaintext APDU.
 * @param plain_len Length of plaintext APDU.
 * @param out_apdu Output buffer for ciphered APDU.
 * @param max_out_len Capacity of out_apdu.
 * @param out_len Output actual length of ciphered APDU.
 * @return DLMS_OK on success, error otherwise.
 */
dlms_result_t dlms_suite0_encrypt_apdu(
    dlms_security_context_t *sec,
    uint8_t sc,
    dlms_apdu_tag_t cipher_tag,
    const uint8_t *plain_apdu, uint16_t plain_len,
    uint8_t *out_apdu, uint16_t max_out_len, uint16_t *out_len);

/**
 * @brief Verify client's HLS 5 GMAC challenge reply f(StoC).
 * @param sec Security context.
 * @param client_reply Client reply data (SC(1) + IC(4) + Tag(12) = 17 bytes).
 * @param reply_len Length of client reply (must be 17).
 * @return true if valid and authentic, false otherwise.
 */
bool dlms_suite0_hls5_verify_reply(
    dlms_security_context_t *sec,
    const uint8_t *client_reply, uint16_t reply_len);

/**
 * @brief Build server's HLS 5 GMAC response f(CtoS) to prove server identity.
 * @param sec Security context.
 * @param out_response Output buffer for server response (17 bytes).
 * @param max_out_len Capacity of out_response.
 * @param actual_out_len Actual length written (17 bytes).
 * @return true on success.
 */
bool dlms_suite0_hls5_build_response(
    dlms_security_context_t *sec,
    uint8_t *out_response, uint16_t max_out_len, uint16_t *actual_out_len);

/**
 * @brief Unwrap a new key transferred in Security Setup (Class 64, Method 2).
 * @param sec Security context.
 * @param wrapped_key Wrapped key data (24 bytes for 16-byte key).
 * @param wrapped_len Length of wrapped key.
 * @param unwrapped_key Output buffer for unwrapped key (16 bytes).
 * @return true on success and integrity verified.
 */
bool dlms_suite0_unwrap_key(
    const dlms_security_context_t *sec,
    const uint8_t *wrapped_key, uint16_t wrapped_len,
    uint8_t *unwrapped_key);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_SECURITY_SUITE0_H */
