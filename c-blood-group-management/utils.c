#include "utils.h"

/*
 * utils.c - Implementation of safe terminal I/O, whitespace trimming,
 * and strict domain validators conforming to Blueprint v3.
 */

/* Time: O(n), Space: O(1) where n is string length */
void utils_trim_whitespace(char *str) {
    char *start;
    char *end;

    if (!str) return;

    start = str;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    if (*start == '\0') {
        *str = '\0';
        return;
    }

    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

/* Time: O(m), Space: O(1) where m is buffer size */
Status utils_read_line(char *buf, size_t size, int *out_eof) {
    if (out_eof) *out_eof = 0;
    if (!buf || size == 0) return ST_ERR_INVALID;

    if (!fgets(buf, (int)size, stdin)) {
        buf[0] = '\0';
        if (out_eof) *out_eof = 1;
        return ST_ERR_STATE; /* EOF encountered */
    }

    size_t len = strlen(buf);
    int newline_found = 0;

    if (len > 0 && buf[len - 1] == '\n') {
        newline_found = 1;
        buf[len - 1] = '\0';
        len--;
        if (len > 0 && buf[len - 1] == '\r') {
            buf[len - 1] = '\0';
            len--;
        }
    }

    if (!newline_found) {
        if (feof(stdin) && len < size - 1) {
            /* Short line without a trailing newline ending right at EOF */
            return ST_OK;
        }

        /* Line was longer than size - 1 characters.
         * Discard the remainder of the line from stdin to avoid polluting future reads. */
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {
            /* discard */
        }
        buf[0] = '\0';
        return ST_ERR_INVALID; /* Report "too long" */
    }

    return ST_OK;
}

/* Time: O(d), Space: O(1) where d is input string length */
Status utils_parse_int(const char *str, int *out_val, int min_val, int max_val) {
    char copy[64];
    char *endptr;
    long val;

    if (!str || !out_val) return ST_ERR_INVALID;

    utils_copy_string(copy, str, sizeof(copy));
    utils_trim_whitespace(copy);

    if (copy[0] == '\0') {
        return ST_ERR_INVALID;
    }

    size_t start = 0;
    if (copy[0] == '+' || copy[0] == '-') {
        start = 1;
    }

    if (copy[start] == '\0') {
        return ST_ERR_INVALID;
    }

    /* Count digits: reject input longer than 9 digits before calling strtol (Hard Constraint 2) */
    size_t digit_count = 0;
    for (size_t i = start; copy[i] != '\0'; i++) {
        if (!isdigit((unsigned char)copy[i])) {
            return ST_ERR_INVALID;
        }
        digit_count++;
    }

    if (digit_count > 9) {
        return ST_ERR_INVALID; /* Overflow prevention */
    }

    val = strtol(copy, &endptr, 10);
    if (*endptr != '\0') {
        return ST_ERR_INVALID;
    }

    if (val < (long)min_val || val > (long)max_val) {
        return ST_ERR_INVALID;
    }

    *out_val = (int)val;
    return ST_OK;
}

/* Time: O(d), Space: O(1) where d is input line length */
Status utils_read_int(int *out_val, int min_val, int max_val, int *out_eof) {
    char line[64];

    if (!out_val) return ST_ERR_INVALID;

    Status st = utils_read_line(line, sizeof(line), out_eof);
    if (st != ST_OK) {
        return st;
    }

    return utils_parse_int(line, out_val, min_val, max_val);
}

/* Time: O(n), Space: O(1) where n is text length */
int utils_is_valid_text(const char *str, size_t max_len) {
    if (!str) return 0;
    size_t len = strlen(str);
    if (len == 0 || len > max_len) {
        return 0;
    }

    /* Must be trimmed: first and last characters cannot be whitespace */
    if (isspace((unsigned char)str[0]) || isspace((unsigned char)str[len - 1])) {
        return 0;
    }

    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        /* Printable ASCII only (32-126) and strictly NO '|' (file delimiter) */
        if (c < 32 || c > 126 || c == '|') {
            return 0;
        }
    }
    return 1;
}

/* Time: O(m), Space: O(1) where m is buffer size */
Status utils_read_text(char *buf, size_t size, size_t max_len, int *out_eof) {
    if (!buf || size == 0 || max_len == 0) return ST_ERR_INVALID;

    Status st = utils_read_line(buf, size, out_eof);
    if (st != ST_OK) {
        return st;
    }

    utils_trim_whitespace(buf);
    if (!utils_is_valid_text(buf, max_len)) {
        return ST_ERR_INVALID;
    }

    return ST_OK;
}

/* Time: O(1) per prompt, Space: O(1) */
Status utils_read_confirm(const char *prompt, int *out_confirmed, int *out_eof) {
    char buf[32];
    if (out_eof) *out_eof = 0;
    if (out_confirmed) *out_confirmed = 0;

    for (;;) {
        if (prompt) {
            printf("%s (y/n): ", prompt);
            fflush(stdout);
        }

        Status st = utils_read_line(buf, sizeof(buf), out_eof);
        if (out_eof && *out_eof) {
            return ST_ERR_STATE; /* Canceled on EOF */
        }
        if (st != ST_OK) {
            printf("Input too long. Please enter 'y' or 'n'.\n");
            continue;
        }

        utils_trim_whitespace(buf);
        if (buf[0] == '\0') {
            printf("Please enter 'y' or 'n'.\n");
            continue;
        }

        if (strcmp(buf, "y") == 0 || strcmp(buf, "Y") == 0 ||
            strcmp(buf, "yes") == 0 || strcmp(buf, "YES") == 0) {
            if (out_confirmed) *out_confirmed = 1;
            return ST_OK;
        }

        if (strcmp(buf, "n") == 0 || strcmp(buf, "N") == 0 ||
            strcmp(buf, "no") == 0 || strcmp(buf, "NO") == 0) {
            if (out_confirmed) *out_confirmed = 0;
            return ST_OK;
        }

        printf("Invalid choice. Please enter 'y' for yes or 'n' for no.\n");
    }
}

/* Time: O(p), Space: O(1) where p is phone length */
int utils_is_valid_phone(const char *phone) {
    if (!phone) return 0;
    size_t len = strlen(phone);
    if (len < MIN_PHONE_DIGITS || len > MAX_PHONE_DIGITS) {
        return 0;
    }
    for (size_t i = 0; i < len; i++) {
        if (!isdigit((unsigned char)phone[i])) {
            return 0;
        }
    }
    return 1;
}

/* Time: O(1), Space: O(1) */
int utils_is_valid_age(int age) {
    return (age >= MIN_DONOR_AGE && age <= MAX_DONOR_AGE);
}

/* Time: O(m), Space: O(1) where m <= dest_size */
void utils_copy_string(char *dest, const char *src, size_t dest_size) {
    if (!dest || dest_size == 0) return;
    if (!src) {
        dest[0] = '\0';
        return;
    }
    size_t i = 0;
    while (i + 1 < dest_size && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/* Time: O(n), Space: O(1) */
int utils_strcasecmp(const char *s1, const char *s2) {
    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;

    while (*s1 && *s2) {
        int c1 = toupper((unsigned char)*s1);
        int c2 = toupper((unsigned char)*s2);
        if (c1 != c2) {
            return c1 - c2;
        }
        s1++;
        s2++;
    }
    return (int)toupper((unsigned char)*s1) - (int)toupper((unsigned char)*s2);
}
