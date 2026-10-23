/**
 * include/rng.h
 * Cryptographic random number generator (C2, F7).
 * Reads /dev/urandom on POSIX, CryptGenRandom on Windows, with fallback to seeded xorshift128+.
 */

#ifndef RNG_H
#define RNG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void rng_init(void);
void rng_bytes(void *buf, size_t len);
uint32_t rng_u32(void);
uint32_t rng_range(uint32_t min, uint32_t max);

#endif /* RNG_H */
