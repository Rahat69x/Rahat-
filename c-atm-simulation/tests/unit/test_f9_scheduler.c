/**
 * tests/unit/test_f9_scheduler.c
 * Automated test suite for F9 Scheduler, interest calculation, standing orders, and EMI auto-debit.
 */

#include "scheduler.h"
#include "bank.h"
#include "account.h"
#include "standing.h"
#include "loans.h"
#include "vouchers.h"
#include "audit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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

static Bank bank1;
static Bank bank2;

int main(void) {
    printf("====================================================\n");
    printf("     RUNNING F9 SCHEDULER & INTEREST TEST SUITE     \n");
    printf("====================================================\n");

    /* 1. 30-Day Interest vs Closed-Form Formula */
    bank_init(&bank1);
    bank1.config.savings_rate_pct = 4;
    account_create(&bank1, 1001, "Interest User", 100000LL, "1234"); /* $1,000.00 */

    double closed_form_interest = 100000.0 * 0.04 * (30.0 / 365.0); /* ~328.767 cents */
    scheduler_advance_days(&bank1, 30);

    Account *acc = account_find(&bank1, 1001);
    TEST_CHECK(acc != NULL, "Account found after 30 days");
    int64_t credited_interest = acc->balance - 100000LL;
    double diff = fabs((double)credited_interest - closed_form_interest);
    TEST_CHECK(diff <= 1.0, "Interest: 30-day credited interest matches closed form within 1 minor unit");
    TEST_CHECK(credited_interest == 328LL, "Interest: Exact credited interest is 328 cents ($3.28)");

    /* 2. Equivalence: 30 single-day advances vs 1 30-day advance */
    bank_init(&bank2);
    bank2.config.savings_rate_pct = 4;
    account_create(&bank2, 1001, "Interest User", 100000LL, "1234");

    for (int d = 0; d < 30; ++d) {
        bank2.sim_day++;
        scheduler_tick(&bank2);
    }

    Account *acc2 = account_find(&bank2, 1001);
    TEST_CHECK(acc2->balance == acc->balance, "Scheduler equivalence: Final balances match exactly");
    TEST_CHECK(acc2->interest_remainder == acc->interest_remainder, "Scheduler equivalence: Exact remainder match");

    /* 3. Idempotency on crash mid-tick */
    int64_t balance_before = acc->balance;
    /* Simulate crash & resume: call scheduler_tick again without advancing sim_day */
    scheduler_tick(&bank1);
    TEST_CHECK(acc->balance == balance_before, "Idempotency: Re-running tick on same sim_day does not double-apply");

    /* 4. Standing order execution & 3-strike failure suspension */
    account_create(&bank1, 1002, "SO Recipient", 5000LL, "1234");
    uint64_t so_id = 0;
    char err[64];
    bool created = standing_create(&bank1, 1001, 1002, 1000LL, FREQ_DAILY, bank1.sim_day + 1, 5, &so_id, err, sizeof(err));
    TEST_CHECK(created, "Standing order created with daily frequency");

    int64_t b1001_prev = acc->balance;
    Account *acc1002 = account_find(&bank1, 1002);
    int64_t b1002_prev = acc1002->balance;

    scheduler_advance_days(&bank1, 1);
    TEST_CHECK(acc1002->balance == b1002_prev + 1000LL, "Standing order debited and credited on due day");
    TEST_CHECK(acc->balance < b1001_prev, "Sender debited for standing order");

    /* Drain sender balance to force standing order failures */
    acc->balance = 500LL; /* less than 1000 */
    scheduler_advance_days(&bank1, 1); /* fail 1 */
    scheduler_advance_days(&bank1, 1); /* fail 2 */
    scheduler_advance_days(&bank1, 1); /* fail 3 -> suspended */

    StandingOrder *so = &bank1.standing_orders[0];
    TEST_CHECK(!so->is_active, "Standing order suspended after 3 consecutive failures");
    TEST_CHECK(bank1.alert_count > 0, "Security alert raised on standing order suspension");

    /* 5. Loan EMI auto-debit and late penalty fee */
    acc->balance = 500000LL; /* $5,000.00 */
    uint64_t loan_id = 0;
    bank1.sim_day = 35;
    acc->created_day = 1;
    bool loan_ok = loan_apply(&bank1, 1001, LOAN_SHORT, 100000LL, 6, &loan_id, err, sizeof(err));
    TEST_CHECK(loan_ok, "Loan application submitted for auto-debit test");

    bool dual = false;
    loan_approve(&bank1, "ADMIN001", loan_id, &dual, err, sizeof(err));
    Loan *ln = loan_find(&bank1, loan_id);
    TEST_CHECK(ln && ln->status == LOAN_ACTIVE, "Loan is active and disbursed");

    /* Fast-forward 30 days to EMI due date */
    int64_t pre_emi_bal = acc->balance;
    scheduler_advance_days(&bank1, 30);
    TEST_CHECK(acc->balance < pre_emi_bal, "Loan EMI auto-debited on 30th day");
    TEST_CHECK(ln->remaining_balance < 100000LL, "Loan remaining balance decreased");

    /* Cause EMI bounce on next cycle */
    acc->balance = 100LL; /* Cannot pay EMI */
    scheduler_advance_days(&bank1, 30);
    TEST_CHECK(ln->missed_emis == 1, "Loan missed EMI counter incremented");
    TEST_CHECK(acc->balance < 100LL, "Late fee charged on bounced EMI");

    /* 6. Voucher expiration on daily tick */
    char vcode[32];
    voucher_create(&bank1, "ADMIN001", VOUCHER_CREDIT, 5000LL, bank1.sim_day + 2, 10, 1, vcode);
    Voucher *v = &bank1.vouchers[bank1.voucher_count - 1];
    TEST_CHECK(v && v->status == VOUCHER_ACTIVE, "Voucher created with expiry in 2 days");
    scheduler_advance_days(&bank1, 3);
    TEST_CHECK(v->status == VOUCHER_EXPIRED, "Voucher expired automatically by scheduler");

    printf("\nTEST RESULTS: %d Passed, %d Failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
