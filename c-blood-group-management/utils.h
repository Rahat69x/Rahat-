#ifndef UTILS_H
#define UTILS_H

#include "common.h"

/*
 * utils.h - Terminal input reading, string normalization, and domain validators.
 * Layer 1 module: depends only on common.h.
 *
 * Hard constraints enforced:
 *  - Input via fgets; newline stripped; remainder of over-long line discarded;
 *    reports ST_ERR_INVALID when line exceeds buffer size.
 *  - Handles EOF cleanly without infinite loops (*out_eof set to 1).
 *  - Integer parsing rejects input longer than 9 digits before strtol (overflow impossible).
 *  - Characters cast to (unsigned char) before any ctype.h call.
 *  - Text fields must be printable ASCII 32-126 only, trimmed, non-empty,
 *    within maximum length, and contain NO '|' delimiter.
 */

/*
 * Reads a single line of input from standard input into buf.
 * Strips trailing '\n' and '\r'.
 * If the line exceeds size - 1 characters, discards the rest of the line
 * from stdin and returns ST_ERR_INVALID.
 * Sets *out_eof to 1 and returns ST_ERR_STATE on EOF.
 * Returns ST_OK on success.
 */
Status utils_read_line(char *buf, size_t size, int *out_eof);

/*
 * Parses an integer from a string within the range [min_val, max_val].
 * Rejects input longer than 9 digits before calling strtol to ensure no overflow.
 * Trims leading/trailing whitespace.
 * Returns ST_OK on success, ST_ERR_INVALID on invalid format or range.
 */
Status utils_parse_int(const char *str, int *out_val, int min_val, int max_val);

/*
 * Reads an integer from standard input within the range [min_val, max_val].
 * Rejects input longer than 9 digits before calling strtol to ensure no overflow.
 * Sets *out_eof to 1 on EOF.
 * Returns ST_OK on success, ST_ERR_INVALID on invalid format/range,
 * or ST_ERR_STATE on EOF.
 */
Status utils_read_int(int *out_val, int min_val, int max_val, int *out_eof);

/*
 * Reads and validates a text string from standard input.
 * Trims leading/trailing whitespace.
 * Enforces printable ASCII 32-126, non-empty, length <= max_len, and NO '|'.
 * Sets *out_eof to 1 on EOF.
 * Returns ST_OK on success, ST_ERR_INVALID on validation failure.
 */
Status utils_read_text(char *buf, size_t size, size_t max_len, int *out_eof);

/*
 * Prompts the user with a question and reads a yes/no response.
 * Sets *out_confirmed to 1 for yes ('y'/'Y'), 0 for no ('n'/'N').
 * Sets *out_eof to 1 on EOF.
 * Returns ST_OK on success, ST_ERR_STATE on EOF.
 */
Status utils_read_confirm(const char *prompt, int *out_confirmed, int *out_eof);

/*
 * In-place trimming of leading and trailing whitespace.
 * Always casts characters to (unsigned char) before isspace().
 */
void utils_trim_whitespace(char *str);

/*
 * Validates text string:
 *  - Must be non-NULL and length > 0
 *  - Length <= max_len
 *  - Only printable ASCII characters (32 to 126 inclusive)
 *  - Must NOT contain the file delimiter '|'
 *  - Must be trimmed (first and last characters cannot be space)
 * Returns 1 if valid, 0 otherwise.
 */
int utils_is_valid_text(const char *str, size_t max_len);

/*
 * Validates a phone number string:
 *  - Digits only ('0'-'9')
 *  - Length between MIN_PHONE_DIGITS (10) and MAX_PHONE_DIGITS (15)
 *  - Leading zeros are preserved
 * Returns 1 if valid, 0 otherwise.
 */
int utils_is_valid_phone(const char *phone);

/*
 * Validates donor age:
 *  - Must be between MIN_DONOR_AGE (18) and MAX_DONOR_AGE (65) inclusive.
 * Returns 1 if valid, 0 otherwise.
 */
int utils_is_valid_age(int age);

/*
 * Safe bounded string copy guaranteed to null-terminate without truncation warnings.
 */
void utils_copy_string(char *dest, const char *src, size_t dest_size);

/*
 * Case-insensitive ASCII string comparison (pure C99, portable across all platforms).
 * Returns <0 if s1 < s2, 0 if s1 == s2, >0 if s1 > s2.
 */
int utils_strcasecmp(const char *s1, const char *s2);

#endif /* UTILS_H */

