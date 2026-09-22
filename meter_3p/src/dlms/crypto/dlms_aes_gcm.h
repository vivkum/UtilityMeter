/**
 * @file dlms_aes_gcm.h
 * @brief AES-GCM-128 and AES-GMAC Authenticated Encryption (NIST SP 800-38D)
 * 
 * Standard for DLMS/COSEM Security Suite 0.
 */

#ifndef DLMS_AES_GCM_H
#define DLMS_AES_GCM_H

#include "dlms_aes.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    dlms_aes128_ctx_t aes_ctx;
    uint8_t h[16];           /**< Subkey H = AES_K(0) */
    uint8_t j0[16];          /**< Initial counter block J0 */
    uint8_t ctr[16];         /**< Current counter block */
    uint8_t ghash[16];       /**< Accumulated GHASH value */
    uint64_t aad_len;        /**< Total AAD length in bytes */
    uint64_t data_len;       /**< Total Plaintext/Ciphertext length in bytes */
    uint8_t buf[16];         /**< Partial block buffer */
    size_t  buf_len;
} dlms_gcm_ctx_t;

/**
 * @brief Initialize GCM context with 128-bit key and IV.
 * @param ctx GCM context.
 * @param key 16-byte AES key.
 * @param iv Initialization vector (typically 12 bytes in DLMS Suite 0).
 * @param iv_len Length of IV in bytes (must be 12).
 */
void dlms_gcm_init(dlms_gcm_ctx_t *ctx, const uint8_t key[16], const uint8_t *iv, size_t iv_len);

/**
 * @brief Feed Additional Authenticated Data (AAD) into GCM.
 */
void dlms_gcm_aad(dlms_gcm_ctx_t *ctx, const uint8_t *aad, size_t aad_len);

/**
 * @brief Encrypt plaintext data in-place or into cipher buffer.
 */
void dlms_gcm_encrypt(dlms_gcm_ctx_t *ctx, const uint8_t *plain, size_t len, uint8_t *cipher);

/**
 * @brief Decrypt ciphertext data in-place or into plain buffer.
 */
void dlms_gcm_decrypt(dlms_gcm_ctx_t *ctx, const uint8_t *cipher, size_t len, uint8_t *plain);

/**
 * @brief Finalize GCM and generate authentication tag.
 * @param ctx GCM context.
 * @param tag Output buffer for tag.
 * @param tag_len Desired tag length (12 bytes for DLMS Suite 0 default, up to 16).
 */
void dlms_gcm_finish(dlms_gcm_ctx_t *ctx, uint8_t *tag, size_t tag_len);

/**
 * @brief One-shot Authenticated Encryption with AES-GCM-128.
 */
bool dlms_aes_gcm_encrypt_and_tag(
    const uint8_t key[16],
    const uint8_t *iv, size_t iv_len,
    const uint8_t *aad, size_t aad_len,
    const uint8_t *plain, size_t plain_len,
    uint8_t *cipher,
    uint8_t *tag, size_t tag_len);

/**
 * @brief One-shot Decrypt and Verify Authenticated AES-GCM-128.
 */
bool dlms_aes_gcm_decrypt_and_verify(
    const uint8_t key[16],
    const uint8_t *iv, size_t iv_len,
    const uint8_t *aad, size_t aad_len,
    const uint8_t *cipher, size_t cipher_len,
    const uint8_t *tag, size_t tag_len,
    uint8_t *plain);

/**
 * @brief Calculate AES-GMAC (Authentication Only, as used in DLMS HLS 5).
 */
bool dlms_aes_gmac(
    const uint8_t key[16],
    const uint8_t *iv, size_t iv_len,
    const uint8_t *data, size_t data_len,
    uint8_t *tag, size_t tag_len);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_AES_GCM_H */
