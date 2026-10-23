/**
 * src/security/crc32.c
 * Handcrafted standard CRC-32 with reflected polynomial 0xEDB88320 (C2, F7).
 */

#include "crc32.h"

static uint32_t s_crc_table[256];
static int s_table_initialized = 0;

static void crc32_init_table(void) {
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int j = 0; j < 8; ++j) {
            c = (c & 1) ? (0xEDB88320L ^ (c >> 1)) : (c >> 1);
        }
        s_crc_table[i] = c;
    }
    s_table_initialized = 1;
}

uint32_t crc32_calculate(const void *data, size_t length) {
    if (!s_table_initialized) {
        crc32_init_table();
    }
    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFL;
    for (size_t i = 0; i < length; ++i) {
        crc = s_crc_table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFL;
}
