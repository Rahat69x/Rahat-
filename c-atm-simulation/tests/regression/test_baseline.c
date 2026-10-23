/**
 * tests/regression/test_baseline.c
 * Regression test suite proving new BankApi local implementation behaves identically
 * to the baseline legacy/atm_basic.c for all core features (Section 1).
 */

#include "bank.h"
#include "bank_api.h"
#include "account.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int g_pass = 0;
static int g_fail = 0;

#define REG_CHECK(cond, name) do { \
    if (cond) { \
        printf("  [PASS] %s\n", name); \
        g_pass++; \
    } else { \
        printf("  [FAIL] %s (Line %d)\n", name, __LINE__); \
        g_fail++; \
    } \
} while(0)

static void test_baseline_features(void) {
    static Bank bank;
    bank_init(&bank);
    bank_seed_demo(&bank);
    BankApi api = bank_api_create_local(&bank);

    /* 1. Verify 3 built-in demo accounts */
    Account *a1 = account_find(&bank, 1001);
    Account *a2 = account_find(&bank, 1002);
    Account *a3 = account_find(&bank, 1003);
    REG_CHECK(a1 && a2 && a3, "Baseline: 3 demo accounts (1001, 1002, 1003) exist");
    REG_CHECK(a1->balance == 5000000LL && a2->balance == 7500000LL && a3->balance == 2500000LL,
              "Baseline: Initial balances match $50k, $75k, $25k");

    /* 2. Authentication and 3-attempt lockout */
    char name[MAX_NAME_LEN + 1];
    ApiResult r = api.login(api.ctx, 1001, "9999", name);
    REG_CHECK(!r.success && a1->failed_pins == 1, "Baseline: Wrong PIN rejected (Attempt 1)");
    api.login(api.ctx, 1001, "9999", name);
    REG_CHECK(a1->failed_pins == 2, "Baseline: Wrong PIN rejected (Attempt 2)");
    api.login(api.ctx, 1001, "9999", name);
    REG_CHECK(a1->failed_pins == 3 && a1->status == STATUS_BLOCKED, "Baseline: 3 wrong PINs blocks card");

    r = api.login(api.ctx, 1001, "1234", name);
    REG_CHECK(!r.success && strstr(r.message, "BLOCKED") != NULL, "Baseline: Blocked card rejects valid PIN");

    /* 3. Successful login on Account 1002 */
    r = api.login(api.ctx, 1002, "2345", name);
    REG_CHECK(r.success && strcmp(name, "Bob Jones") == 0, "Baseline: Account 1002 authenticates with PIN 2345");

    /* 4. Check Balance */
    int64_t bal = 0;
    r = api.get_balance(api.ctx, 1002, &bal);
    REG_CHECK(r.success && bal == 7500000LL, "Baseline: Balance check returns exact minor units ($75,000.00)");

    /* 5. Deposit */
    r = api.deposit(api.ctx, 1002, 500000LL); /* +$5,000 */
    REG_CHECK(r.success && r.balance_after == 8000000LL, "Baseline: Deposit $5,000 increases balance to $80,000.00");
    REG_CHECK(strlen(r.receipt_no) > 0, "Baseline: Deposit issues transaction receipt");

    /* 6. Withdrawal: multiples of 100, limits, insufficient funds */
    r = api.withdraw(api.ctx, 1002, 5050); /* Non-multiple of 100 dollars */
    REG_CHECK(!r.success, "Baseline: Non-multiple of $100 rejected");

    r = api.withdraw(api.ctx, 1002, 2500000LL); /* $25,000 > per-withdraw limit $20,000 */
    REG_CHECK(!r.success, "Baseline: Exceeding $20,000 per-withdrawal limit rejected");

    r = api.withdraw(api.ctx, 1002, 1000000LL); /* $10,000 withdrawal */
    REG_CHECK(r.success && r.balance_after == 7000000LL, "Baseline: Valid $10,000 withdrawal succeeds");

    /* 7. Transfer */
    r = api.transfer(api.ctx, 1002, 1003, 1500000LL, ""); /* $15,000 from 1002 to 1003 */
    REG_CHECK(r.success && r.balance_after == 5500000LL, "Baseline: Sender debited $15,000");
    REG_CHECK(a3->balance == 4000000LL, "Baseline: Recipient credited $15,000");

    /* 8. History: Last 5 transactions */
    Txn hist[10];
    size_t count = 0;
    r = api.get_history(api.ctx, 1002, hist, 5, &count);
    REG_CHECK(r.success && count >= 3, "Baseline: Transaction history records returned");

    /* 9. Change PIN */
    r = api.change_pin(api.ctx, 1002, "2345", "9876");
    REG_CHECK(r.success, "Baseline: PIN changed to 9876");
    r = api.login(api.ctx, 1002, "2345", name);
    REG_CHECK(!r.success, "Baseline: Old PIN 2345 no longer valid");
    r = api.login(api.ctx, 1002, "9876", name);
    REG_CHECK(r.success, "Baseline: New PIN 9876 logs in successfully");

    bank_destroy(&bank);
}

int main(void) {
    printf("====================================================\n");
    printf("     RUNNING BASELINE ATM REGRESSION TEST SUITE     \n");
    printf("====================================================\n");

    test_baseline_features();

    printf("====================================================\n");
    printf("REGRESSION SUMMARY: %d PASSED, %d FAILED (TOTAL %d)\n", g_pass, g_fail, g_pass + g_fail);
    printf("====================================================\n");

    return (g_fail == 0) ? 0 : 1;
}
