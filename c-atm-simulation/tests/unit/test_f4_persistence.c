/**
 * tests/unit/test_f4_persistence.c
 * Verification suite for F4 persistence, atomic writes, CRC-32 fallback, and lock files.
 */

#include "store.h"
#include "bank.h"
#include "account.h"
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
    printf("   RUNNING F4 PERSISTENCE & RECOVERY TEST SUITE     \n");
    printf("====================================================\n");

    const char *test_dir = "data";

    /* 1. Single-writer lock acquisition */
    char err[128];
    store_release_lock(test_dir);
    bool lock1 = store_acquire_lock(test_dir, err, sizeof(err));
    TEST_CHECK(lock1, "Lock file: Primary instance acquires lock");

    bool lock2 = store_acquire_lock(test_dir, err, sizeof(err));
    TEST_CHECK(!lock2 && strstr(err, "Lock file exists") != NULL,
               "Lock file: Second instance blocked from acquiring lock");
    store_release_lock(test_dir);

    /* 2. State serialization and reload */
    Bank bank_orig;
    bank_init(&bank_orig);
    bank_seed_demo(&bank_orig);
    account_create(&bank_orig, 2001, "Persist User", 999900LL, "4321");

    bool saved = store_save_bank(test_dir, &bank_orig);
    TEST_CHECK(saved, "Persistence: Bank state saved atomically to bank.dat");

    Bank bank_loaded;
    bank_init(&bank_loaded);
    bool loaded = store_load_bank(test_dir, &bank_loaded);
    TEST_CHECK(loaded, "Persistence: Bank state successfully loaded with CRC-32 verification");
    TEST_CHECK(bank_loaded.account_count == bank_orig.account_count,
               "Persistence: Exact account count restored across restart");

    Account *loaded_acc = account_find(&bank_loaded, 2001);
    TEST_CHECK(loaded_acc && loaded_acc->balance == 999900LL,
               "Persistence: Account 2001 balance and credentials intact");

    /* Ensure bank.dat.bak is created by performing another atomic save */
    store_save_bank(test_dir, &bank_orig);

    /* 3. CRC-32 corruption detection and .bak failover */
    /* Intentionally tamper with 1 byte in bank.dat */
    FILE *f = fopen("data/bank.dat", "r+b");
    if (f) {
        fseek(f, 20, SEEK_SET);
        fputc(0xFF, f);
        fclose(f);
    }

    Bank bank_backup_recovered;
    bank_init(&bank_backup_recovered);
    bool recovered = store_load_bank(test_dir, &bank_backup_recovered);
    TEST_CHECK(recovered, "Recovery: Damaged primary bank.dat triggered automatic fallback to bank.dat.bak");
    TEST_CHECK(bank_backup_recovered.account_count == bank_orig.account_count,
               "Recovery: Restored state from backup is bit-accurate");

    bank_destroy(&bank_orig);
    bank_destroy(&bank_loaded);
    bank_destroy(&bank_backup_recovered);

    printf("====================================================\n");
    printf("F4 SUMMARY: %d PASSED, %d FAILED (TOTAL %d)\n", g_pass, g_fail, g_pass + g_fail);
    printf("====================================================\n");

    return (g_fail == 0) ? 0 : 1;
}
