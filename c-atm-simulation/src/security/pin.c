/**
 * src/security/pin.c
 * Salted iterated PIN hashing and constant-time verification (C5, F7).
 */

#include "pin.h"
#include "sha256.h"
#include "rng.h"
#include <string.h>

void pin_generate_salt(uint8_t salt[SALT_SIZE]) {
    rng_bytes(salt, SALT_SIZE);
}

void pin_hash_derive(const char *pin, const uint8_t salt[SALT_SIZE], int iterations, uint8_t hash_out[HASH_SIZE]) {
    if (iterations < 1) iterations = 1;
    Sha256Ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, salt, SALT_SIZE);
    sha256_update(&ctx, pin, strlen(pin));
    sha256_final(&ctx, hash_out);

    for (int i = 1; i < iterations; ++i) {
        sha256_init(&ctx);
        sha256_update(&ctx, hash_out, HASH_SIZE);
        sha256_update(&ctx, salt, SALT_SIZE);
        sha256_final(&ctx, hash_out);
    }
}

int pin_const_time_memcmp(const void *a, const void *b, size_t len) {
    const uint8_t *ua = (const uint8_t *)a;
    const uint8_t *ub = (const uint8_t *)b;
    uint8_t diff = 0;
    for (size_t i = 0; i < len; ++i) {
        diff |= (ua[i] ^ ub[i]);
    }
    return (int)diff;
}

bool pin_verify(const char *pin, const uint8_t salt[SALT_SIZE], int iterations, const uint8_t expected_hash[HASH_SIZE]) {
    uint8_t derived[HASH_SIZE];
    pin_hash_derive(pin, salt, iterations, derived);
    bool match = (pin_const_time_memcmp(derived, expected_hash, HASH_SIZE) == 0);
    
    /* Clean sensitive buffer */
    volatile uint8_t *p = (volatile uint8_t *)derived;
    for (size_t i = 0; i < HASH_SIZE; ++i) p[i] = 0;

    return match;
}
