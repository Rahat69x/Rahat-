/**
 * tests/unit/test_f10_statements.c
 * Automated test suite for F10 Statements, property-based reconciliation (>= 200 histories),
 * and RFC 4180 CSV escaping roundtrip.
 */

#include "statements.h"
#include "analytics.h"
#include "bank.h"
#include "account.h"
#include "txn.h"
#include <stdio.h>
#include <stdlib.h>
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

static Bank test_bank;

/* Simple PRNG for deterministic property test */
static uint32_t prng_state = 123456789;
static uint32_t test_rand(void) {
    prng_state ^= prng_state << 13;
    prng_state ^= prng_state >> 17;
    prng_state ^= prng_state << 5;
    return prng_state;
}

int main(void) {
    printf("====================================================\n");
    printf("     RUNNING F10 STATEMENTS & ANALYTICS TEST SUITE  \n");
    printf("====================================================\n");

    /* 1. RFC 4180 CSV Escaping & Round-Trip */
    const char *test_notes[] = {
        "Simple clean note",
        "Note, with, commas",
        "Note with \"quotes\" inside",
        "Combined, \"quoted and commas\", notes",
        "\"Completely quoted field\"",
        "Special chars: !@#$%^&*()",
        "Multiple \"\"double\"\" quotes"
    };
    int num_notes = sizeof(test_notes) / sizeof(test_notes[0]);
    bool rfc_all_ok = true;

    for (int i = 0; i < num_notes; ++i) {
        char escaped[256];
        char unescaped[256];
        statement_rfc4180_escape(test_notes[i], escaped, sizeof(escaped));
        statement_rfc4180_unescape(escaped, unescaped, sizeof(unescaped));
        if (strcmp(test_notes[i], unescaped) != 0) {
            printf("  [FAIL] RFC 4180 roundtrip mismatch: orig='%s', unesc='%s'\n", test_notes[i], unescaped);
            rfc_all_ok = false;
        }
    }
    TEST_CHECK(rfc_all_ok, "RFC 4180: All fields containing commas and quotes round-trip correctly");

    /* 2. Property Test: Reconciliation holds for >= 200 generated random histories */
    int property_histories = 250;
    int recon_passed = 0;

    for (int h = 0; h < property_histories; ++h) {
        bank_init(&test_bank);
        long acc_id = 9000 + (h % 50);
        int64_t init_bal = 100000LL + (test_rand() % 500000LL);
        account_create(&test_bank, acc_id, "Recon User", init_bal, "1234");
        Account *acc = account_find(&test_bank, acc_id);

        int tx_count = 10 + (test_rand() % 30);
        int cur_day = 1;

        for (int t = 0; t < tx_count; ++t) {
            test_bank.sim_day = cur_day;
            if (test_rand() % 3 == 0) cur_day += 1;

            int64_t amt = (10 + (test_rand() % 1000)) * 100LL;
            int type_choice = test_rand() % 4;

            if (type_choice == 0) {
                /* Deposit */
                acc->balance += amt;
                txn_record(&test_bank, acc_id, TXN_DEPOSIT, amt, acc->balance, 0, "Deposit test", NULL);
            } else if (type_choice == 1) {
                /* Withdraw */
                if (acc->balance >= amt) {
                    acc->balance -= amt;
                    txn_record(&test_bank, acc_id, TXN_WITHDRAW, amt, acc->balance, 0, "Withdraw test", NULL);
                }
            } else if (type_choice == 2) {
                /* Transfer In */
                acc->balance += amt;
                txn_record(&test_bank, acc_id, TXN_TRANSFER_IN, amt, acc->balance, 8888, "Transfer In", NULL);
            } else {
                /* Transfer Out */
                if (acc->balance >= amt) {
                    acc->balance -= amt;
                    txn_record(&test_bank, acc_id, TXN_TRANSFER_OUT, amt, acc->balance, 8888, "Transfer Out", NULL);
                }
            }
        }

        Statement stmt;
        int s_day = 1 + (test_rand() % (cur_day > 1 ? cur_day : 2));
        int e_day = s_day + (test_rand() % 10);
        statement_generate(&test_bank, acc_id, s_day, e_day, &stmt);

        if (stmt.reconciled && (stmt.opening_balance + stmt.total_in - stmt.total_out == stmt.closing_balance)) {
            recon_passed++;
        }
    }

    TEST_CHECK(recon_passed == property_histories,
               "Property Test: Reconciliation holds for 250/250 randomly generated transaction histories");

    /* 3. Statement Export to Disk (TXT and CSV) */
    bank_init(&test_bank);
    account_create(&test_bank, 1001, "Alice Smith", 5000000LL, "1234");
    Account *a1001 = account_find(&test_bank, 1001);
    a1001->balance += 50000LL;
    txn_record(&test_bank, 1001, TXN_DEPOSIT, 50000LL, a1001->balance, 0, "Salary, \"Bonus\"", NULL);

    Statement stmt;
    statement_generate(&test_bank, 1001, 1, 30, &stmt);
    char txt_path[128], csv_path[128];
    bool txt_ok = statement_export_txt(&test_bank, &stmt, "statements", txt_path, sizeof(txt_path));
    bool csv_ok = statement_export_csv(&stmt, "statements", csv_path, sizeof(csv_path));
    TEST_CHECK(txt_ok, "Export: Text statement exported to statements/");
    TEST_CHECK(csv_ok, "Export: RFC 4180 CSV statement exported to statements/");

    /* 4. Budgets and 80%/100% threshold evaluation */
    budget_set_limit(&test_bank, 1001, TXN_WITHDRAW, 100000LL); /* $1,000 budget */
    a1001->balance -= 85000LL;
    txn_record(&test_bank, 1001, TXN_WITHDRAW, 85000LL, a1001->balance, 0, "ATM cash", NULL);
    budget_evaluate(&test_bank, 1001);
    TEST_CHECK(test_bank.alert_count > 0 && strstr(test_bank.alerts[test_bank.alert_count - 1].message, "80%"),
               "Budgets: 80% threshold notice generated");

    a1001->balance -= 20000LL;
    txn_record(&test_bank, 1001, TXN_WITHDRAW, 20000LL, a1001->balance, 0, "ATM cash 2", NULL);
    budget_evaluate(&test_bank, 1001);
    TEST_CHECK(test_bank.alert_count > 1 && strstr(test_bank.alerts[test_bank.alert_count - 1].message, "100%"),
               "Budgets: 100% threshold exceeded alert generated");

    printf("\nTEST RESULTS: %d Passed, %d Failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
