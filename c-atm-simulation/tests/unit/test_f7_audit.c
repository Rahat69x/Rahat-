/**
 * tests/unit/test_f7_audit.c
 * Verification suite for F7 tamper-evident SHA-256 audit log hash chaining.
 */

#include "audit.h"
#include "bank.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int g_pass = 0;
static int g_fail = 0;

#define TEST_CHECK(cond, name) do { \
    if (cond) { \
        printf("  [PASS] %s\n", name); \
        g_pass++; \
    } else { \
        printf("  [FAIL] %s (Line %d)\n", name, __LINE__); \
        g_fail++; \
    } \
} while(0)

int main(void) {
    printf("====================================================\n");
    printf("     RUNNING F7 AUDIT LOG INTEGRITY TEST SUITE      \n");
    printf("====================================================\n");

    const char *test_dir = "data";
    remove("data/audit.log");

    static Bank bank;
    bank_init(&bank);
    audit_init(&bank);

    /* 1. Append valid audit events */
    audit_record(&bank, test_dir, "SYSTEM", "BOOT", "-", "System initialization");
    audit_record(&bank, test_dir, "1001", "LOGIN_OK", "-", "Customer logged in");
    audit_record(&bank, test_dir, "1001", "WITHDRAW", "ATM", "Dispensed $100");
    audit_record(&bank, test_dir, "ADMIN001", "CARD_UNBLOCK", "1002", "Unblocked customer card");

    uint64_t broken = 0;
    char reason[128];
    bool valid = audit_verify(test_dir, bank.audit_head_hash, &broken, reason, sizeof(reason));
    TEST_CHECK(valid, "Audit Chain: Valid hash chain passes verification");

    /* 2. Tamper with line 2 in audit.log */
    FILE *f = fopen("data/audit.log", "r+");
    if (f) {
        /* Seek to character 'W' in line 3 ("WITHDRAW") and change to 'X' */
        char buf[512];
        fgets(buf, sizeof(buf), f); /* line 1 */
        fgets(buf, sizeof(buf), f); /* line 2 */
        long pos = ftell(f);
        fseek(f, pos + 10, SEEK_SET);
        fputc('Z', f);
        fclose(f);
    }

    bool tampered = audit_verify(test_dir, bank.audit_head_hash, &broken, reason, sizeof(reason));
    TEST_CHECK(!tampered && broken == 3, "Audit Chain: Byte tamper at sequence 3 accurately identified");

    bank_destroy(&bank);

    printf("====================================================\n");
    printf("F7 AUDIT SUMMARY: %d PASSED, %d FAILED (TOTAL %d)\n", g_pass, g_fail, g_pass + g_fail);
    printf("====================================================\n");

    return (g_fail == 0) ? 0 : 1;
}
