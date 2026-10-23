/**
 * include/pin.h
 * Salted, iterated PIN hashing with constant-time verification (C5, F7).
 */

#ifndef PIN_H
#define PIN_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "types.h"

void pin_generate_salt(uint8_t salt[SALT_SIZE]);
void pin_hash_derive(const char *pin, const uint8_t salt[SALT_SIZE], int iterations, uint8_t hash_out[HASH_SIZE]);
bool pin_verify(const char *pin, const uint8_t salt[SALT_SIZE], int iterations, const uint8_t expected_hash[HASH_SIZE]);
int pin_const_time_memcmp(const void *a, const void *b, size_t len);

#endif /* PIN_H */
