/**
 * include/crc32.h
 * Standard CRC-32 (IEEE 802.3 / ISO 3309, polynomial 0xEDB88320) (C2, F7).
 */

#ifndef CRC32_H
#define CRC32_H

#include <stdint.h>
#include <stddef.h>

uint32_t crc32_calculate(const void *data, size_t length);

#endif /* CRC32_H */
