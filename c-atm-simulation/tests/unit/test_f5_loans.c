/**
 * tests/unit/test_f5_loans.c
 * Verification suite for F5 Loans, compound EMI calculation, schedule rounding absorption, and approval.
 */

#include "loans.h"
#include "bank.h"
#include "account.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
    printf("         RUNNING F5 LOAN LIFECYCLE TEST SUITE       \n");
    printf("====================================================\n");

    static Bank bank;
    bank_init(&bank);
    bank_seed_demo(&bank);
    bank.sim_day = 35; /* Account age >= 30 days */

    /* 1. Acceptance Vector: EMI for 100,000 at 12% over 12 months is 8884.88 (±1 minor unit) */
    /* 100,000.00 currency units = 10,000,000 minor units. Expected EMI: 888,488 minor units ($8884.88) */
    int64_t principal_100k = 10000000LL;
    int64_t emi_100k = loan_calculate_emi(principal_100k, 12, 12);
    int64_t expected_emi = 888488LL;
    int64_t diff = llabs(emi_100k - expected_emi);
    TEST_CHECK(diff <= 1, "Loan Acceptance: EMI for 100,000 at 12% over 12 months is 8884.88 (±1 unit)");

    /* 2. Amortization Schedule: sum of principal parts equals principal, ends at exactly 0 */
    AmortizationRow rows[12];
    bool sched = loan_generate_schedule(principal_100k, 12, 12, rows, 12);
    TEST_CHECK(sched, "Amortization: Schedule generated for 12 months");

    int64_t total_principal_paid = 0;
    int64_t total_interest_paid = 0;
    for (int i = 0; i < 12; ++i) {
        total_principal_paid += rows[i].principal_part;
        total_interest_paid += rows[i].interest_part;
    }
    TEST_CHECK(total_principal_paid == principal_100k,
               "Loan Acceptance: Amortization schedule sums to initial principal");
    TEST_CHECK(total_interest_paid > 0,
               "Loan Acceptance: Total interest paid is non-zero and accumulated");
    TEST_CHECK(rows[11].balance_after == 0,
               "Loan Acceptance: Final installment absorbs rounding and reaches exactly 0");

    /* 3. Loan Application Eligibility */
    Account *acc = account_find(&bank, 1001);
    char err[128];
    uint64_t loan_id = 0;

    /* Single approval loan: $30,000 (< loan_dual_approval_limit = $50,000) */
    int64_t small_principal = 3000000LL;
    bool app1 = loan_apply(&bank, acc->id, LOAN_PERSONAL, small_principal, 12, &loan_id, err, sizeof(err));
    TEST_CHECK(app1 && loan_id > 0, "Loan Application: Eligible customer successfully applies");

    Loan *ln1 = loan_find(&bank, loan_id);
    TEST_CHECK(ln1 && ln1->status == LOAN_PENDING, "Loan Flow: Initial status is PENDING");

    /* 4. Single Admin Approval */
    bool needs_second = false;
    int64_t bal_before = acc->balance;
    bool app_res = loan_approve(&bank, "ADMIN001", loan_id, &needs_second, err, sizeof(err));
    TEST_CHECK(app_res && !needs_second && ln1->status == LOAN_ACTIVE,
               "Loan Approval: Single admin approves loan under dual-approval threshold");
    TEST_CHECK(acc->balance == bal_before + small_principal,
               "Loan Disbursement: Principal disbursed directly to customer balance");

    /* 5. Dual Approval for Large Loans (> $50,000) */
    uint64_t big_loan_id = 0;
    int64_t big_principal = 6000000LL; /* $60,000 */
    bool app2 = loan_apply(&bank, acc->id, LOAN_PERSONAL, big_principal, 24, &big_loan_id, err, sizeof(err));
    TEST_CHECK(app2, "Loan Application: Large loan applied");

    /* First admin approves */
    loan_approve(&bank, "ADMIN001", big_loan_id, &needs_second, err, sizeof(err));
    TEST_CHECK(needs_second, "Loan Dual-Approval: First approval flags need for second admin");

    /* Same admin cannot approve twice */
    bool same_admin = loan_approve(&bank, "ADMIN001", big_loan_id, &needs_second, err, sizeof(err));
    TEST_CHECK(!same_admin, "Loan Dual-Approval: Same administrator rejected for second approval");

    /* Second admin approves */
    bool second_ok = loan_approve(&bank, "ADMIN002", big_loan_id, &needs_second, err, sizeof(err));
    Loan *big_loan = loan_find(&bank, big_loan_id);
    TEST_CHECK(second_ok && !needs_second && big_loan->status == LOAN_ACTIVE,
               "Loan Dual-Approval: Second distinct admin successfully completes approval");

    /* 6. Prepayment (Full & Partial) */
    /* Partial prepay $10,000 on small loan */
    int64_t rem_before = ln1->remaining_balance;
    bool prep_part = loan_prepay(&bank, acc->id, ln1->id, 1000000LL, false, err, sizeof(err));
    TEST_CHECK(prep_part && ln1->remaining_balance == rem_before - 1000000LL,
               "Loan Prepay: Partial prepayment reduces principal balance");

    /* Full prepay closes loan */
    bool prep_full = loan_prepay(&bank, acc->id, ln1->id, 0, true, err, sizeof(err));
    TEST_CHECK(prep_full && ln1->remaining_balance == 0 && ln1->status == LOAN_PAID_OFF,
               "Loan Prepay: Full prepayment closes loan with status PAID_OFF");

    printf("\nSUMMARY: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail > 0 ? 1 : 0;
}
