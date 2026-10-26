#ifndef TESTING_H
#define TESTING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * tests/testing.h - Standalone unit test framework for pure C99 without assert.h.
 * Uses only permitted standard headers (stdio.h, stdlib.h, string.h).
 */

static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define CHECK(cond) do { \
    g_tests_run++; \
    if (cond) { \
        g_tests_passed++; \
    } else { \
        g_tests_failed++; \
        printf("  [FAIL] %s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

#define CHECK_EQ_INT(actual, expected) do { \
    g_tests_run++; \
    int _act = (int)(actual); \
    int _exp = (int)(expected); \
    if (_act == _exp) { \
        g_tests_passed++; \
    } else { \
        g_tests_failed++; \
        printf("  [FAIL] %s:%d: expected %d, got %d (%s == %s)\n", \
               __FILE__, __LINE__, _exp, _act, #actual, #expected); \
    } \
} while (0)

#define CHECK_STR_EQ(actual, expected) do { \
    g_tests_run++; \
    const char *_s1 = (actual); \
    const char *_s2 = (expected); \
    if (_s1 && _s2 && strcmp(_s1, _s2) == 0) { \
        g_tests_passed++; \
    } else { \
        g_tests_failed++; \
        printf("  [FAIL] %s:%d: expected \"%s\", got \"%s\"\n", \
               __FILE__, __LINE__, _s2 ? _s2 : "(null)", _s1 ? _s1 : "(null)"); \
    } \
} while (0)

#define TEST_INIT(suite_name) do { \
    printf("--- Running %s ---\n", suite_name); \
} while (0)

#define TEST_REPORT() do { \
    printf("\nTotal assertions: %d | Passed: %d | Failed: %d\n", \
           g_tests_run, g_tests_passed, g_tests_failed); \
    if (g_tests_failed == 0) { \
        printf("[SUCCESS] All tests passed.\n"); \
    } else { \
        printf("[FAILURE] %d assertion(s) failed.\n", g_tests_failed); \
    } \
} while (0)

#endif /* TESTING_H */
