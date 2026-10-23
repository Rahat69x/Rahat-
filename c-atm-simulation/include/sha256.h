/**
 * include/sha256.h
 * Handcrafted SHA-256 implementation conforming to FIPS 180-4 (C2, F7).
 */

#ifndef SHA256_H
#define SHA256_H

#include <stdint.h>
#include <stddef.h>

#define SHA256_BLOCK_SIZE 64
#define SHA256_DIGEST_SIZE 32

typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t buffer[SHA256_BLOCK_SIZE];
} Sha256Ctx;

void sha256_init(Sha256Ctx *ctx);
void sha256_update(Sha256Ctx *ctx, const void *data, size_t len);
void sha256_final(Sha256Ctx *ctx, uint8_t digest[SHA256_DIGEST_SIZE]);
void sha256_hash(const void *data, size_t len, uint8_t digest[SHA256_DIGEST_SIZE]);
void sha256_to_hex(const uint8_t digest[SHA256_DIGEST_SIZE], char hex_out[65]);

#endif /* SHA256_H */
