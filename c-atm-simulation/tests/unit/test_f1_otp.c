/**
 * tests/unit/test_f1_otp.c
 * Verification suite for F1 Simulated OTP codes, TTL expiration, retry limit, and resends.
 */

#include "otp.h"
#include "bank.h"
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
    printf("        RUNNING F1 SIMULATED OTP TEST SUITE         \n");
    printf("====================================================\n");

    static Bank bank;
    bank_init(&bank);
    otp_init();

    time_t base_time = 1700000000;
    clock_set_mock(base_time, 1);

    long acc_id = 1001;
    char code[8];
    char err[128];

    /* 1. Issue OTP */
    bool issued = otp_issue(&bank, acc_id, OTP_PURPOSE_TRANSFER, 500000LL, code);
    TEST_CHECK(issued && strlen(code) == 6, "OTP Issue: 6-digit cryptographic code generated");

    /* 2. Wrong code attempts decrement tries */
    bool v_wrong = otp_verify(&bank, acc_id, OTP_PURPOSE_TRANSFER, 500000LL, "000000", err, sizeof(err));
    TEST_CHECK(!v_wrong, "OTP Verify: Incorrect code rejected");

    /* 3. Valid code verification succeeds */
    bool v_ok = otp_verify(&bank, acc_id, OTP_PURPOSE_TRANSFER, 500000LL, code, err, sizeof(err));
    TEST_CHECK(v_ok, "OTP Verify: Correct code verified successfully");

    /* 4. Single-use: Reused code must be rejected */
    bool v_reuse = otp_verify(&bank, acc_id, OTP_PURPOSE_TRANSFER, 500000LL, code, err, sizeof(err));
    TEST_CHECK(!v_reuse, "OTP Acceptance: Single-use enforced (reused code rejected)");

    /* 5. TTL expiration */
    otp_issue(&bank, acc_id, OTP_PURPOSE_PIN_CHANGE, 0, code);
    /* Advance time beyond otp_ttl_s (120s) */
    clock_set_mock(base_time + 130, 1);
    bool v_exp = otp_verify(&bank, acc_id, OTP_PURPOSE_PIN_CHANGE, 0, code, err, sizeof(err));
    TEST_CHECK(!v_exp && strstr(err, "expired") != NULL, "OTP Acceptance: Expired code rejected via injectable clock");

    /* 6. Wrong code x 3 cancels session */
    clock_set_mock(base_time + 200, 1);
    otp_issue(&bank, acc_id, OTP_PURPOSE_LOAN, 1000000LL, code);
    otp_verify(&bank, acc_id, OTP_PURPOSE_LOAN, 1000000LL, "111111", err, sizeof(err));
    otp_verify(&bank, acc_id, OTP_PURPOSE_LOAN, 1000000LL, "222222", err, sizeof(err));
    bool v_third = otp_verify(&bank, acc_id, OTP_PURPOSE_LOAN, 1000000LL, "333333", err, sizeof(err));
    TEST_CHECK(!v_third && strstr(err, "exhausted") != NULL, "OTP Acceptance: 3 wrong tries cancels operation");

    /* 7. Resend limits and gap */
    clock_set_mock(base_time + 400, 1);
    otp_issue(&bank, acc_id, OTP_PURPOSE_TRANSFER, 200000LL, code);

    /* Immediate resend before gap (30s) fails */
    clock_set_mock(base_time + 410, 1);
    char new_code[8];
    bool r_fast = otp_resend(&bank, acc_id, OTP_PURPOSE_TRANSFER, 200000LL, new_code, err, sizeof(err));
    TEST_CHECK(!r_fast && strstr(err, "wait") != NULL, "OTP Resend: Resend before 30s gap rejected");

    /* Resend after gap succeeds (resend 1) */
    clock_set_mock(base_time + 435, 1);
    bool r1 = otp_resend(&bank, acc_id, OTP_PURPOSE_TRANSFER, 200000LL, new_code, err, sizeof(err));
    TEST_CHECK(r1 && strlen(new_code) == 6, "OTP Resend: Resend after gap succeeds (1/2)");

    /* Resend 2 */
    clock_set_mock(base_time + 470, 1);
    bool r2 = otp_resend(&bank, acc_id, OTP_PURPOSE_TRANSFER, 200000LL, new_code, err, sizeof(err));
    TEST_CHECK(r2, "OTP Resend: Resend 2/2 succeeds");

    /* Resend 3 exceeds max (otp_resend_max = 2) */
    clock_set_mock(base_time + 510, 1);
    bool r3 = otp_resend(&bank, acc_id, OTP_PURPOSE_TRANSFER, 200000LL, new_code, err, sizeof(err));
    TEST_CHECK(!r3 && strstr(err, "Maximum") != NULL, "OTP Resend: Exceeding max resends rejected");

    printf("\nSUMMARY: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail > 0 ? 1 : 0;
}
