/**
 * src/core/money.c
 * Integer arithmetic for money in minor units (C4).
 */

#include "money.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

bool money_add(int64_t a, int64_t b, int64_t *out) {
    if (!out) return false;
    if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)) {
        return false;
    }
    *out = a + b;
    return true;
}

bool money_sub(int64_t a, int64_t b, int64_t *out) {
    if (!out) return false;
    if ((b < 0 && a > INT64_MAX + b) || (b > 0 && a < INT64_MIN + b)) {
        return false;
    }
    *out = a - b;
    return true;
}

bool money_mul(int64_t a, int64_t b, int64_t *out) {
    if (!out) return false;
    if (a == 0 || b == 0) {
        *out = 0;
        return true;
    }
    if (a > 0) {
        if (b > 0 && a > INT64_MAX / b) return false;
        if (b < 0 && b < INT64_MIN / a) return false;
    } else {
        if (b > 0 && a < INT64_MIN / b) return false;
        if (b < 0 && b < INT64_MAX / a) return false;
    }
    *out = a * b;
    return true;
}

bool money_format(int64_t minor_units, const char *symbol, char *out, size_t out_len) {
    if (!out || out_len == 0) return false;
    const char *sym = symbol ? symbol : "$";
    bool neg = minor_units < 0;
    int64_t abs_units = neg ? -minor_units : minor_units;
    int64_t major = abs_units / 100;
    int64_t cents = abs_units % 100;

    int written;
    if (neg) {
        written = snprintf(out, out_len, "-%s%lld.%02lld", sym, (long long)major, (long long)cents);
    } else {
        written = snprintf(out, out_len, "%s%lld.%02lld", sym, (long long)major, (long long)cents);
    }
    return (written > 0 && (size_t)written < out_len);
}

bool money_parse_minor(const char *str, int64_t *out_minor) {
    if (!str || !out_minor) return false;
    while (isspace((unsigned char)*str)) str++;
    if (*str == '$') str++;
    while (isspace((unsigned char)*str)) str++;

    char *endptr = NULL;
    double val = strtod(str, &endptr);
    if (endptr == str || val < 0.0) return false;
    while (*endptr != '\0') {
        if (!isspace((unsigned char)*endptr)) return false;
        endptr++;
    }
    *out_minor = (int64_t)(val * 100.0 + 0.5);
    return true;
}
