/**
 * include/input.h
 * Bounded safe input parser replacing gets/scanf/strcpy/sprintf (C3).
 */

#ifndef INPUT_H
#define INPUT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

bool input_get_line(char *buf, size_t max_len);
bool input_get_int(int *out_val, int min_val, int max_val);
bool input_get_long(long *out_val, long min_val, long max_val);
bool input_get_int64(int64_t *out_val, int64_t min_val, int64_t max_val);
bool input_get_masked_pin(char *buf, size_t max_len);

#endif /* INPUT_H */
