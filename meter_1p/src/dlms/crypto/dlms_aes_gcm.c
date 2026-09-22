/**
 * @file dlms_aes_gcm.c
 * @brief AES-GCM-128 and AES-GMAC Implementation (NIST SP 800-38D)
 */

#include "dlms_aes_gcm.h"
#include <string.h>

/* GF(2^128) multiplication with reduction polynomial R = 0xE100000000000000... */
static void ghash_mult(uint8_t x[16], const uint8_t h[16]) {
    uint8_t z[16] = {0};
    uint8_t v[16];
    memcpy(v, h, 16);

    for (int i = 0; i < 16; i++) {
        for (int bit = 7; bit >= 0; bit--) {
            if ((x[i] >> bit) & 1) {
                for (int j = 0; j < 16; j++) {
                    z[j] ^= v[j];
                }
            }
            uint8_t lsb = v[15] & 1;
            for (int j = 15; j > 0; j--) {
                v[j] = (uint8_t)((v[j] >> 1) | ((v[j - 1] & 1) << 7));
            }
            v[0] >>= 1;
            if (lsb) {
                v[0] ^= 0xe1;
            }
        }
    }
    memcpy(x, z, 16);
}

static void ghash_block(dlms_gcm_ctx_t *ctx, const uint8_t block[16]) {
    for (int i = 0; i < 16; i++) {
        ctx->ghash[i] ^= block[i];
    }
    ghash_mult(ctx->ghash, ctx->h);
}

static void ctr32_inc(uint8_t ctr[16]) {
    for (int i = 15; i >= 12; i--) {
        if (++ctr[i] != 0) {
            break;
        }
    }
}

void dlms_gcm_init(dlms_gcm_ctx_t *ctx, const uint8_t key[16], const uint8_t *iv, size_t iv_len) {
    memset(ctx, 0, sizeof(dlms_gcm_ctx_t));
    dlms_aes128_set_key_enc(&ctx->aes_ctx, key);

    /* H = AES_K(0) */
    uint8_t zero[16] = {0};
    dlms_aes128_encrypt_block(&ctx->aes_ctx, zero, ctx->h);

    if (iv_len == 12) {
        /* Standard 96-bit IV: J0 = IV || 0x00000001 */
        memcpy(ctx->j0, iv, 12);
        ctx->j0[12] = 0;
        ctx->j0[13] = 0;
        ctx->j0[14] = 0;
        ctx->j0[15] = 1;
    } else {
        /* Generic IV length: J0 = GHASH_H(IV || 0^(s+64) || [len(IV)]_64) */
        size_t full_blocks = iv_len / 16;
        for (size_t i = 0; i < full_blocks; i++) {
            ghash_block(ctx, iv + i * 16);
        }
        size_t rem = iv_len % 16;
        if (rem > 0) {
            uint8_t pad[16] = {0};
            memcpy(pad, iv + full_blocks * 16, rem);
            ghash_block(ctx, pad);
        }
        uint8_t len_blk[16] = {0};
        uint64_t bit_len = (uint64_t)iv_len * 8;
        for (int i = 0; i < 8; i++) {
            len_blk[15 - i] = (uint8_t)(bit_len >> (i * 8));
        }
        ghash_block(ctx, len_blk);
        memcpy(ctx->j0, ctx->ghash, 16);
        memset(ctx->ghash, 0, 16);
    }

    memcpy(ctx->ctr, ctx->j0, 16);
    ctr32_inc(ctx->ctr); /* CTR starts at J0 + 1 for encryption */
}

void dlms_gcm_aad(dlms_gcm_ctx_t *ctx, const uint8_t *aad, size_t aad_len) {
    ctx->aad_len += aad_len;
    while (aad_len > 0) {
        if (ctx->buf_len == 0 && aad_len >= 16) {
            ghash_block(ctx, aad);
            aad += 16;
            aad_len -= 16;
        } else {
            size_t take = 16 - ctx->buf_len;
            if (take > aad_len) take = aad_len;
            memcpy(ctx->buf + ctx->buf_len, aad, take);
            ctx->buf_len += take;
            aad += take;
            aad_len -= take;
            if (ctx->buf_len == 16) {
                ghash_block(ctx, ctx->buf);
                ctx->buf_len = 0;
            }
        }
    }
}

static void gcm_aad_pad(dlms_gcm_ctx_t *ctx) {
    if (ctx->buf_len > 0) {
        memset(ctx->buf + ctx->buf_len, 0, 16 - ctx->buf_len);
        ghash_block(ctx, ctx->buf);
        ctx->buf_len = 0;
    }
}

void dlms_gcm_encrypt(dlms_gcm_ctx_t *ctx, const uint8_t *plain, size_t len, uint8_t *cipher) {
    if (ctx->data_len == 0 && ctx->buf_len > 0) {
        gcm_aad_pad(ctx);
    }
    ctx->data_len += len;

    while (len > 0) {
        uint8_t ectr[16];
        dlms_aes128_encrypt_block(&ctx->aes_ctx, ctx->ctr, ectr);
        ctr32_inc(ctx->ctr);

        size_t chunk = (len > 16) ? 16 : len;
        uint8_t c_block[16] = {0};
        for (size_t i = 0; i < chunk; i++) {
            cipher[i] = plain[i] ^ ectr[i];
            c_block[i] = cipher[i];
        }

        ghash_block(ctx, c_block);

        plain += chunk;
        cipher += chunk;
        len -= chunk;
    }
}

void dlms_gcm_decrypt(dlms_gcm_ctx_t *ctx, const uint8_t *cipher, size_t len, uint8_t *plain) {
    if (ctx->data_len == 0 && ctx->buf_len > 0) {
        gcm_aad_pad(ctx);
    }
    ctx->data_len += len;

    while (len > 0) {
        uint8_t ectr[16];
        dlms_aes128_encrypt_block(&ctx->aes_ctx, ctx->ctr, ectr);
        ctr32_inc(ctx->ctr);

        size_t chunk = (len > 16) ? 16 : len;
        uint8_t c_block[16] = {0};
        for (size_t i = 0; i < chunk; i++) {
            c_block[i] = cipher[i];
            plain[i] = cipher[i] ^ ectr[i];
        }

        ghash_block(ctx, c_block);

        plain += chunk;
        cipher += chunk;
        len -= chunk;
    }
}

void dlms_gcm_finish(dlms_gcm_ctx_t *ctx, uint8_t *tag, size_t tag_len) {
    if (ctx->buf_len > 0) {
        gcm_aad_pad(ctx);
    }

    /* Append [len(A)]_64 || [len(C)]_64 in bits */
    uint8_t len_block[16];
    uint64_t aad_bits = ctx->aad_len * 8;
    uint64_t data_bits = ctx->data_len * 8;

    for (int i = 0; i < 8; i++) {
        len_block[7 - i]  = (uint8_t)(aad_bits >> (i * 8));
        len_block[15 - i] = (uint8_t)(data_bits >> (i * 8));
    }

    ghash_block(ctx, len_block);

    /* T = MSB_t(GHASH ^ AES_K(J0)) */
    uint8_t ej0[16];
    dlms_aes128_encrypt_block(&ctx->aes_ctx, ctx->j0, ej0);

    for (int i = 0; i < 16; i++) {
        ctx->ghash[i] ^= ej0[i];
    }

    size_t copy_len = (tag_len > 16) ? 16 : tag_len;
    memcpy(tag, ctx->ghash, copy_len);
}

bool dlms_aes_gcm_encrypt_and_tag(
    const uint8_t key[16],
    const uint8_t *iv, size_t iv_len,
    const uint8_t *aad, size_t aad_len,
    const uint8_t *plain, size_t plain_len,
    uint8_t *cipher,
    uint8_t *tag, size_t tag_len)
{
    dlms_gcm_ctx_t ctx;
    dlms_gcm_init(&ctx, key, iv, iv_len);
    if (aad && aad_len > 0) {
        dlms_gcm_aad(&ctx, aad, aad_len);
    }
    if (plain && plain_len > 0) {
        dlms_gcm_encrypt(&ctx, plain, plain_len, cipher);
    }
    dlms_gcm_finish(&ctx, tag, tag_len);
    return true;
}

bool dlms_aes_gcm_decrypt_and_verify(
    const uint8_t key[16],
    const uint8_t *iv, size_t iv_len,
    const uint8_t *aad, size_t aad_len,
    const uint8_t *cipher, size_t cipher_len,
    const uint8_t *tag, size_t tag_len,
    uint8_t *plain)
{
    dlms_gcm_ctx_t ctx;
    dlms_gcm_init(&ctx, key, iv, iv_len);
    if (aad && aad_len > 0) {
        dlms_gcm_aad(&ctx, aad, aad_len);
    }
    if (cipher && cipher_len > 0) {
        dlms_gcm_decrypt(&ctx, cipher, cipher_len, plain);
    }
    uint8_t computed_tag[16];
    dlms_gcm_finish(&ctx, computed_tag, tag_len);

    /* Constant-time tag comparison to prevent timing attacks */
    uint8_t diff = 0;
    for (size_t i = 0; i < tag_len; i++) {
        diff |= (computed_tag[i] ^ tag[i]);
    }
    return (diff == 0);
}

bool dlms_aes_gmac(
    const uint8_t key[16],
    const uint8_t *iv, size_t iv_len,
    const uint8_t *data, size_t data_len,
    uint8_t *tag, size_t tag_len)
{
    dlms_gcm_ctx_t ctx;
    dlms_gcm_init(&ctx, key, iv, iv_len);
    if (data && data_len > 0) {
        dlms_gcm_aad(&ctx, data, data_len);
    }
    dlms_gcm_finish(&ctx, tag, tag_len);
    return true;
}
