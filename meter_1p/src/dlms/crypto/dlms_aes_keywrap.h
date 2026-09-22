/**
 * @file dlms_aes_keywrap.h
 * @brief AES Key Wrap Algorithm (NIST SP 800-38F / RFC 3394)
 * 
 * Used for secure key transfer in DLMS Security Setup (Class 64, Method 2).
 */

#ifndef DLMS_AES_KEYWRAP_H
#define DLMS_AES_KEYWRAP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Wrap (encrypt) a cryptographic key using a Key Encrypting Key (KEK).
 * @param kek 16-byte Master Key / KEK.
 * @param plain_key Plaintext key to wrap (must be multiple of 8 bytes, e.g. 16 bytes for AES-128).
 * @param key_len Length of plain_key (e.g. 16).
 * @param wrapped_out Output buffer for wrapped key (must be key_len + 8 bytes).
 * @return true on success, false on error.
 */
bool dlms_aes_key_wrap(
    const uint8_t kek[16],
    const uint8_t *plain_key, size_t key_len,
    uint8_t *wrapped_out);

/**
 * @brief Unwrap (decrypt and verify) a wrapped key using KEK.
 * @param kek 16-byte Master Key / KEK.
 * @param wrapped_data Wrapped key data (length must be multiple of 8, >= 24 bytes).
 * @param wrapped_len Length of wrapped_data.
 * @param plain_key_out Output buffer for recovered plaintext key (must be wrapped_len - 8 bytes).
 * @return true on success and integrity check passed, false on tampering or error.
 */
bool dlms_aes_key_unwrap(
    const uint8_t kek[16],
    const uint8_t *wrapped_data, size_t wrapped_len,
    uint8_t *plain_key_out);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_AES_KEYWRAP_H */
