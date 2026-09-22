/**
 * @file dlms_aes_keywrap.c
 * @brief AES Key Wrap Implementation (NIST SP 800-38F / RFC 3394)
 */

#include "dlms_aes_keywrap.h"
#include "dlms_aes.h"
#include <string.h>

#define KEYWRAP_IV_BYTE 0xA6

bool dlms_aes_key_wrap(
    const uint8_t kek[16],
    const uint8_t *plain_key, size_t key_len,
    uint8_t *wrapped_out)
{
    if (!kek || !plain_key || !wrapped_out) return false;
    if (key_len < 16 || (key_len % 8) != 0) return false;

    size_t n = key_len / 8;
    dlms_aes128_ctx_t ctx;
    dlms_aes128_set_key_enc(&ctx, kek);

    /* Initialize A = IV (0xA6A6A6A6A6A6A6A6) */
    uint8_t a[8];
    memset(a, KEYWRAP_IV_BYTE, 8);

    /* Copy plaintext key into R[1]..R[n] */
    uint8_t r[8 * 8]; /* Support keys up to 64 bytes */
    if (n > 8) return false;
    memcpy(r, plain_key, key_len);

    for (int j = 0; j <= 5; j++) {
        for (size_t i = 1; i <= n; i++) {
            uint8_t in_blk[16];
            uint8_t out_blk[16];

            memcpy(in_blk, a, 8);
            memcpy(in_blk + 8, &r[(i - 1) * 8], 8);

            dlms_aes128_encrypt_block(&ctx, in_blk, out_blk);

            memcpy(a, out_blk, 8);
            memcpy(&r[(i - 1) * 8], out_blk + 8, 8);

            /* A ^= (n * j + i) */
            uint64_t t = (uint64_t)(n * j + i);
            for (int k = 0; k < 8; k++) {
                a[7 - k] ^= (uint8_t)(t >> (k * 8));
            }
        }
    }

    memcpy(wrapped_out, a, 8);
    memcpy(wrapped_out + 8, r, key_len);
    return true;
}

bool dlms_aes_key_unwrap(
    const uint8_t kek[16],
    const uint8_t *wrapped_data, size_t wrapped_len,
    uint8_t *plain_key_out)
{
    if (!kek || !wrapped_data || !plain_key_out) return false;
    if (wrapped_len < 24 || (wrapped_len % 8) != 0) return false;

    size_t n = (wrapped_len - 8) / 8;
    if (n > 8) return false;

    dlms_aes128_ctx_t ctx;
    dlms_aes128_set_key_dec(&ctx, kek);

    uint8_t a[8];
    memcpy(a, wrapped_data, 8);

    uint8_t r[8 * 8];
    memcpy(r, wrapped_data + 8, n * 8);

    for (int j = 5; j >= 0; j--) {
        for (size_t i = n; i >= 1; i--) {
            /* A ^= (n * j + i) */
            uint64_t t = (uint64_t)(n * j + i);
            for (int k = 0; k < 8; k++) {
                a[7 - k] ^= (uint8_t)(t >> (k * 8));
            }

            uint8_t in_blk[16];
            uint8_t out_blk[16];

            memcpy(in_blk, a, 8);
            memcpy(in_blk + 8, &r[(i - 1) * 8], 8);

            dlms_aes128_decrypt_block(&ctx, in_blk, out_blk);

            memcpy(a, out_blk, 8);
            memcpy(&r[(i - 1) * 8], out_blk + 8, 8);
        }
    }

    /* Integrity check: A must equal 0xA6A6A6A6A6A6A6A6 */
    for (int k = 0; k < 8; k++) {
        if (a[k] != KEYWRAP_IV_BYTE) {
            return false; /* Integrity verification failed! */
        }
    }

    memcpy(plain_key_out, r, n * 8);
    return true;
}
