/**
 * @file dlms_aes.h
 * @brief AES-128 Block Cipher Implementation (FIPS 197)
 */

#ifndef DLMS_AES_H
#define DLMS_AES_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DLMS_AES_BLOCK_SIZE 16
#define DLMS_AES_ROUNDS     10

typedef struct {
    uint32_t round_keys[4 * (DLMS_AES_ROUNDS + 1)];
} dlms_aes128_ctx_t;

/** Initialize AES-128 context for encryption */
void dlms_aes128_set_key_enc(dlms_aes128_ctx_t *ctx, const uint8_t key[16]);

/** Initialize AES-128 context for decryption */
void dlms_aes128_set_key_dec(dlms_aes128_ctx_t *ctx, const uint8_t key[16]);

/** Encrypt a single 16-byte block */
void dlms_aes128_encrypt_block(const dlms_aes128_ctx_t *ctx, const uint8_t in[16], uint8_t out[16]);

/** Decrypt a single 16-byte block */
void dlms_aes128_decrypt_block(const dlms_aes128_ctx_t *ctx, const uint8_t in[16], uint8_t out[16]);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_AES_H */
