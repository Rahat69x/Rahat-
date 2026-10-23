/**
 * src/security/hmac.c
 * Handcrafted HMAC-SHA256 according to RFC 2104 (C2, F7).
 */

#include "hmac.h"
#include <string.h>

void hmac_sha256(const void *key, size_t key_len,
                 const void *data, size_t data_len,
                 uint8_t out[HMAC_SHA256_SIZE]) {
    uint8_t k_pad[SHA256_BLOCK_SIZE];
    uint8_t tk[SHA256_DIGEST_SIZE];

    if (key_len > SHA256_BLOCK_SIZE) {
        sha256_hash(key, key_len, tk);
        key = tk;
        key_len = SHA256_DIGEST_SIZE;
    }

    /* Inner hash */
    memset(k_pad, 0x36, sizeof(k_pad));
    for (size_t i = 0; i < key_len; ++i) {
        k_pad[i] ^= ((const uint8_t *)key)[i];
    }

    Sha256Ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, k_pad, sizeof(k_pad));
    sha256_update(&ctx, data, data_len);
    uint8_t inner_digest[SHA256_DIGEST_SIZE];
    sha256_final(&ctx, inner_digest);

    /* Outer hash */
    memset(k_pad, 0x5c, sizeof(k_pad));
    for (size_t i = 0; i < key_len; ++i) {
        k_pad[i] ^= ((const uint8_t *)key)[i];
    }

    sha256_init(&ctx);
    sha256_update(&ctx, k_pad, sizeof(k_pad));
    sha256_update(&ctx, inner_digest, sizeof(inner_digest));
    sha256_final(&ctx, out);
}
