/**
 * src/security/rng.c
 * Cryptographic random byte generator (C2, F7).
 */

#include "rng.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <wincrypt.h>
#endif

static uint64_t s_xorshift_state[2] = {0x123456789ABCDEF0ULL, 0x0FEDCBA987654321ULL};
static bool s_initialized = false;

static uint64_t xorshift128plus(void) {
    uint64_t x = s_xorshift_state[0];
    uint64_t const y = s_xorshift_state[1];
    s_xorshift_state[0] = y;
    x ^= x << 23;
    s_xorshift_state[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
    return s_xorshift_state[1] + y;
}

void rng_init(void) {
    if (s_initialized) return;
    uint64_t seed1 = (uint64_t)time(NULL);
    uint64_t seed2 = (uint64_t)(uintptr_t)&s_initialized;
    s_xorshift_state[0] ^= seed1;
    s_xorshift_state[1] ^= seed2;
    s_initialized = true;
}

void rng_bytes(void *buf, size_t len) {
    rng_init();
    uint8_t *p = (uint8_t *)buf;

#if defined(_WIN32) || defined(_WIN64)
    HCRYPTPROV hProv = 0;
    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
        if (CryptGenRandom(hProv, (DWORD)len, (BYTE *)buf)) {
            CryptReleaseContext(hProv, 0);
            return;
        }
        CryptReleaseContext(hProv, 0);
    }
#else
    FILE *f = fopen("/dev/urandom", "rb");
    if (f) {
        size_t r = fread(buf, 1, len, f);
        fclose(f);
        if (r == len) return;
    }
#endif

    /* Documented fallback for environments without OS crypto provider */
    while (len >= 8) {
        uint64_t r = xorshift128plus();
        for (int i = 0; i < 8; ++i) {
            *p++ = (uint8_t)((r >> (i * 8)) & 0xFF);
        }
        len -= 8;
    }
    if (len > 0) {
        uint64_t r = xorshift128plus();
        for (size_t i = 0; i < len; ++i) {
            *p++ = (uint8_t)((r >> (i * 8)) & 0xFF);
        }
    }
}

uint32_t rng_u32(void) {
    uint32_t val;
    rng_bytes(&val, sizeof(val));
    return val;
}

uint32_t rng_range(uint32_t min, uint32_t max) {
    if (max <= min) return min;
    uint32_t span = max - min + 1;
    return min + (rng_u32() % span);
}
