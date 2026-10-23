/**
 * include/money.h
 * 64-bit integer money manipulation in minor units (1/100th currency) (C4).
 */

#ifndef MONEY_H
#define MONEY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

bool money_add(int64_t a, int64_t b, int64_t *out);
bool money_sub(int64_t a, int64_t b, int64_t *out);
bool money_mul(int64_t a, int64_t b, int64_t *out);

bool money_format(int64_t minor_units, const char *symbol, char *out, size_t out_len);
bool money_parse_minor(const char *str, int64_t *out_minor);

#endif /* MONEY_H */
