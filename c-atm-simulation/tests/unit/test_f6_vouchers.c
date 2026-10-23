/**
 * tests/unit/test_f6_vouchers.c
 * Verification suite for F6 Voucher generation, checksum validation, and redemption limits.
 */

#include "vouchers.h"
#include "bank.h"
#include "account.h"
#include "clock.h"
#include <stdio.h>
#include <string.h>

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
    printf("        RUNNING F6 VOUCHER SYSTEM TEST SUITE        \n");
    printf("====================================================\n");

    Bank bank;
    bank_init(&bank);
    bank_seed_demo(&bank);

    clock_set_mock(1700000000, 1);

    /* 1. Format and Checksum Generation */
    char code[16];
    bool gen = voucher_generate_code(code);
    TEST_CHECK(gen && strlen(code) == 14, "Voucher Gen: Generates XXXX-XXXX-XXXX code");

    char clean[13];
    bool valid = voucher_validate_code_format(code, clean);
    TEST_CHECK(valid && strlen(clean) == 12, "Voucher Checksum: Generated code passes checksum verification");

    /* 2. Typo rejection before lookup */
    /* Tamper with check character (last char) */
    char bad_code[16];
    snprintf(bad_code, sizeof(bad_code), "%s", code);
    bad_code[13] = (bad_code[13] == 'A') ? 'B' : 'A';
    bool bad_check = voucher_validate_code_format(bad_code, clean);
    TEST_CHECK(!bad_check, "Voucher Acceptance: Bad check character rejected without lookup");

    /* 3. Create credit voucher (value: $50 = 5000 minor units, max 2 uses, 1 per account) */
    char v_code[16];
    bool cr = voucher_create(&bank, "ADMIN001", VOUCHER_CREDIT, 5000LL, 10, 2, 1, v_code);
    TEST_CHECK(cr, "Voucher Creation: Credit voucher created with hashed code");

    /* Verify plain code is not in memory of bank */
    TEST_CHECK(memcmp(bank.vouchers[0].code_hash, v_code, 14) != 0,
               "Voucher Security: Code stored as SHA-256 hash, not plain text");

    /* 4. Redeem by Account 1001 */
    Account *a1 = account_find(&bank, 1001);
    int64_t bal_before = a1->balance;
    char msg[128];
    bool red1 = voucher_redeem(&bank, 1001, v_code, msg, sizeof(msg));
    TEST_CHECK(red1 && a1->balance == bal_before + 5000LL,
               "Voucher Redemption: Account 1001 redeemed $50 credit voucher");

    /* 5. Per-account limit enforced */
    bool red_dup = voucher_redeem(&bank, 1001, v_code, msg, sizeof(msg));
    TEST_CHECK(!red_dup && strstr(msg, "already redeemed") != NULL,
               "Voucher Acceptance: Per-account limit enforced (second use by 1001 rejected)");

    /* 6. Account 1002 can use second remaining use */
    Account *a2 = account_find(&bank, 1002);
    int64_t bal2_before = a2->balance;
    bool red2 = voucher_redeem(&bank, 1002, v_code, msg, sizeof(msg));
    TEST_CHECK(red2 && a2->balance == bal2_before + 5000LL,
               "Voucher Redemption: Account 1002 redeemed second use");

    /* 7. Used up voucher rejected */
    bool red_used = voucher_redeem(&bank, 1003, v_code, msg, sizeof(msg));
    TEST_CHECK(!red_used && strstr(msg, "maximum uses") != NULL,
               "Voucher Acceptance: Used-up voucher rejected for subsequent accounts");

    /* 8. Expired voucher rejected */
    char exp_code[16];
    voucher_create(&bank, "ADMIN001", VOUCHER_CREDIT, 2000LL, 5 /* expires day 5 */, 5, 1, exp_code);
    bank.sim_day = 6;
    bool red_exp = voucher_redeem(&bank, 1003, exp_code, msg, sizeof(msg));
    TEST_CHECK(!red_exp && strstr(msg, "expired") != NULL,
               "Voucher Acceptance: Expired voucher rejected");

    /* 9. Voided voucher rejected */
    char void_code[16];
    voucher_create(&bank, "ADMIN001", VOUCHER_CREDIT, 2000LL, 30, 5, 1, void_code);
    bank.vouchers[bank.voucher_count - 1].status = VOUCHER_VOID;
    bool red_void = voucher_redeem(&bank, 1003, void_code, msg, sizeof(msg));
    TEST_CHECK(!red_void && strstr(msg, "voided") != NULL,
               "Voucher Acceptance: Voided voucher rejected");

    /* 10. Bad tries lockout (5 bad attempts locks for 30 min) */
    bank.sim_day = 1;
    for (int i = 0; i < bank.config.voucher_bad_tries; ++i) {
        voucher_redeem(&bank, 1003, "9999-9999-9999", msg, sizeof(msg));
    }
    bool red_locked = voucher_redeem(&bank, 1003, v_code, msg, sizeof(msg));
    TEST_CHECK(!red_locked && strstr(msg, "Locked") != NULL,
               "Voucher Acceptance: 5 bad voucher attempts locks entry for 30 minutes");

    printf("\nSUMMARY: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail > 0 ? 1 : 0;
}
