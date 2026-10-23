/**
 * tests/unit/test_f7_crypto.c
 * Verification suite for F7 primitives against official FIPS/RFC test vectors.
 */

#include "sha256.h"
#include "hmac.h"
#include "crc32.h"
#include "rng.h"
#include "pin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int g_pass = 0;
static int g_fail = 0;

#define TEST_CHECK(cond, name) do { \
    if (cond) { \
        printf("  [PASS] %s\n", name); \
        g_pass++; \
    } else { \
        printf("  [FAIL] %s (Line %d)\n", name, __LINE__); \
        g_fail++; \
    } \
} while(0)

static void test_sha256_vectors(void) {
    uint8_t d[SHA256_DIGEST_SIZE];
    char hex[65];

    /* Vector 1: SHA256("") */
    sha256_hash("", 0, d);
    sha256_to_hex(d, hex);
    TEST_CHECK(strcmp(hex, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855") == 0,
               "SHA-256 Empty String Test Vector");

    /* Vector 2: SHA256("abc") */
    sha256_hash("abc", 3, d);
    sha256_to_hex(d, hex);
    TEST_CHECK(strcmp(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") == 0,
               "SHA-256 'abc' Test Vector");
}

static void test_hmac_vector(void) {
    uint8_t out[HMAC_SHA256_SIZE];
    char hex[65];
    const char *key = "Jefe";
    const char *data = "what do ya want for nothing?";

    hmac_sha256(key, strlen(key), data, strlen(data), out);
    sha256_to_hex(out, hex);
    TEST_CHECK(strcmp(hex, "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843") == 0,
               "HMAC-SHA256 RFC 4231 Vector (Jefe)");
}

static void test_crc32_vector(void) {
    const char *data = "123456789";
    uint32_t val = crc32_calculate(data, strlen(data));
    TEST_CHECK(val == 0xCBF43926, "CRC-32 Standard '123456789' Vector (0xCBF43926)");
}

static void test_pin_hashing(void) {
    uint8_t salt1[SALT_SIZE], salt2[SALT_SIZE];
    pin_generate_salt(salt1);
    pin_generate_salt(salt2);

    TEST_CHECK(pin_const_time_memcmp(salt1, salt2, SALT_SIZE) != 0, "RNG generated distinct 16-byte salts");

    uint8_t h1[HASH_SIZE], h2[HASH_SIZE];
    pin_hash_derive("1234", salt1, 1000, h1);
    pin_hash_derive("1234", salt2, 1000, h2);

    TEST_CHECK(pin_const_time_memcmp(h1, h2, HASH_SIZE) != 0, "Distinct salts produce distinct hashes for same PIN");
    TEST_CHECK(pin_verify("1234", salt1, 1000, h1), "Correct PIN verifies successfully");
    TEST_CHECK(!pin_verify("9999", salt1, 1000, h1), "Incorrect PIN fails verification");
}

int main(void) {
    printf("====================================================\n");
    printf("  RUNNING F7 CRYPTOGRAPHIC PRIMITIVES TEST SUITE   \n");
    printf("====================================================\n");

    test_sha256_vectors();
    test_hmac_vector();
    test_crc32_vector();
    test_pin_hashing();

    printf("====================================================\n");
    printf("CRYPTO SUMMARY: %d PASSED, %d FAILED (TOTAL %d)\n", g_pass, g_fail, g_pass + g_fail);
    printf("====================================================\n");

    return (g_fail == 0) ? 0 : 1;
}
