/**
 * include/hmac.h
 * Handcrafted HMAC-SHA256 according to RFC 2104 (C2, F7).
 */

#ifndef HMAC_H
#define HMAC_H

#include <stdint.h>
#include <stddef.h>
#include "sha256.h"

#define HMAC_SHA256_SIZE 32

void hmac_sha256(const void *key, size_t key_len,
                 const void *data, size_t data_len,
                 uint8_t out[HMAC_SHA256_SIZE]);

#endif /* HMAC_H */
