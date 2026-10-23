/**
 * tests/unit/test_f2_fraud.c
 * Verification suite for F2 Fraud engine rules R1-R6, alerts, and automatic freeze.
 */

#include "fraud.h"
#include "bank.h"
#include "account.h"
#include "clock.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

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
    printf("        RUNNING F2 FRAUD RULE ENGINE TEST SUITE     \n");
    printf("====================================================\n");

    static Bank bank;
    bank_init(&bank);
    bank_seed_demo(&bank);
    fraud_init(&bank);

    /* Construct daytime (12:00 PM local time) to ensure odd hours rule does NOT fire */
    struct tm tm_day = {0};
    tm_day.tm_year = 126; /* 2026 */
    tm_day.tm_mon = 9;
    tm_day.tm_mday = 24;
    tm_day.tm_hour = 12;
    tm_day.tm_min = 0;
    time_t base_time = mktime(&tm_day);
    clock_set_mock(base_time, 1);

    Account *acc = account_find(&bank, 1001);
    TEST_CHECK(acc != NULL, "Fraud Test: Demo account 1001 loaded");
    acc->risk_score = 0;

    /* 1. Rule R2: Large amount (>= 50% daily_withdraw_limit = $25,000 / $50,000) */
    int pts = fraud_eval_operation(&bank, acc, TXN_WITHDRAW, 3000000LL, 0);
    TEST_CHECK(pts == 20 && acc->risk_score == 20, "Rule R2: Large amount adds 20 risk points");

    /* 2. Rule R3: New payee transfer >= threshold ($10,000) */
    clock_set_mock(base_time + 400, 1); /* 5 min past to allow next rule */
    pts = fraud_eval_operation(&bank, acc, TXN_TRANSFER_OUT, 1200000LL, 9999);
    TEST_CHECK(pts == 25 && acc->risk_score == 45, "Rule R3: New payee adds 25 points (Total: 45 >= warn)");
    TEST_CHECK(bank.alert_count > 0, "Fraud Alert: Warning notice generated at score >= 40");

    /* 3. Rule R4: Odd hours (01:00 to 05:00 local time) */
    struct tm tm_odd = tm_day;
    tm_odd.tm_hour = 2; /* 02:30 AM */
    tm_odd.tm_min = 30;
    time_t odd_time = mktime(&tm_odd);
    clock_set_mock(odd_time, 1);

    pts = fraud_eval_operation(&bank, acc, TXN_WITHDRAW, 10000LL, 0);
    TEST_CHECK(pts == 10 && acc->risk_score == 55, "Rule R4: Odd hours (02:30) adds 10 risk points");

    /* Return to daytime for R5 and R6 to avoid R4 triggering */
    struct tm tm_afternoon = tm_day;
    tm_afternoon.tm_hour = 14; /* 02:00 PM */
    time_t day_time2 = mktime(&tm_afternoon);
    clock_set_mock(day_time2, 1);

    /* 4. Rule R5: Authentication / verification failures (+20) */
    fraud_record_failure(&bank, acc->id, "Wrong PIN entered");
    TEST_CHECK(acc->risk_score == 75, "Rule R5: Auth failure adds 20 points (Total: 75 >= stepup)");
    TEST_CHECK(fraud_is_stepup_required(&bank, acc), "Fraud Action: Step-up OTP required when score >= 70");

    /* 5. Rule R6: Drain (takes >= 95% of balance) */
    clock_set_mock(day_time2 + 400, 1);
    acc->balance = 2000000LL; /* $20,000 balance: 96% is $19,200 (< 50% daily limit of $25,000) */
    int64_t drain_amt = (acc->balance * 96) / 100;
    pts = fraud_eval_operation(&bank, acc, TXN_WITHDRAW, drain_amt, 0);
    TEST_CHECK(pts == 20 && acc->risk_score == 95, "Rule R6: Drain adds 20 points (Total: 95 >= freeze)");

    /* 6. Automatic Freeze Check */
    TEST_CHECK(acc->status == STATUS_FROZEN, "Fraud Action: Account automatically FROZEN at risk >= 90");
    TEST_CHECK(fraud_is_frozen(acc), "Fraud Action: fraud_is_frozen returns true");

    /* 7. Frozen accounts cannot do money operations */
    char reason[64];
    bool can_w = account_can_withdraw(&bank, acc, 10000LL, reason, sizeof(reason));
    TEST_CHECK(!can_w, "Fraud Enforcement: Frozen account cannot withdraw funds");

    /* 8. Daily risk score decay */
    int before_decay = acc->risk_score;
    fraud_decay_daily(&bank);
    TEST_CHECK(acc->risk_score == before_decay - 10, "Risk Decay: Risk score decayed by 10 points on daily tick");

    printf("\nSUMMARY: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail > 0 ? 1 : 0;
}
