/**
 * src/ui/input.c
 * Safe input helpers replacing gets/scanf (C3).
 */

#include "input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

bool input_get_line(char *buf, size_t max_len) {
    if (!buf || max_len == 0) return false;
    if (!fgets(buf, (int)max_len, stdin)) return false;
    buf[strcspn(buf, "\r\n")] = '\0';
    return true;
}

bool input_get_int(int *out_val, int min_val, int max_val) {
    if (!out_val) return false;
    char line[64];
    if (!input_get_line(line, sizeof(line))) return false;
    char *endptr = NULL;
    long val = strtol(line, &endptr, 10);
    if (endptr == line || val < min_val || val > max_val) return false;
    *out_val = (int)val;
    return true;
}

bool input_get_long(long *out_val, long min_val, long max_val) {
    if (!out_val) return false;
    char line[64];
    if (!input_get_line(line, sizeof(line))) return false;
    char *endptr = NULL;
    long val = strtol(line, &endptr, 10);
    if (endptr == line || val < min_val || val > max_val) return false;
    *out_val = val;
    return true;
}

bool input_get_int64(int64_t *out_val, int64_t min_val, int64_t max_val) {
    if (!out_val) return false;
    char line[64];
    if (!input_get_line(line, sizeof(line))) return false;
    char *endptr = NULL;
    long long val = strtoll(line, &endptr, 10);
    if (endptr == line || val < min_val || val > max_val) return false;
    *out_val = (int64_t)val;
    return true;
}

bool input_get_masked_pin(char *buf, size_t max_len) {
    if (!buf || max_len == 0) return false;
    size_t idx = 0;

#if defined(_WIN32) || defined(_WIN64)
    while (idx < max_len - 1) {
        int ch = _getch();
        if (ch == 0 || ch == 224) {
            _getch();
            continue;
        }
        if (ch == '\r' || ch == '\n') break;
        if (ch == '\b') {
            if (idx > 0) {
                idx--;
                printf("\b \b");
                fflush(stdout);
            }
        } else if (isdigit(ch)) {
            buf[idx++] = (char)ch;
            putchar('*');
            fflush(stdout);
        }
    }
    buf[idx] = '\0';
    putchar('\n');
    return (idx > 0);
#else
    struct termios old_term, new_term;
    if (tcgetattr(STDIN_FILENO, &old_term) != 0) {
        return input_get_line(buf, max_len);
    }
    new_term = old_term;
    new_term.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    while (idx < max_len - 1) {
        int ch = getchar();
        if (ch == EOF || ch == '\n' || ch == '\r') break;
        if (ch == 127 || ch == '\b') {
            if (idx > 0) {
                idx--;
                printf("\b \b");
                fflush(stdout);
            }
        } else if (isdigit(ch)) {
            buf[idx++] = (char)ch;
            putchar('*');
            fflush(stdout);
        }
    }
    buf[idx] = '\0';
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    putchar('\n');
    return (idx > 0);
#endif
}
