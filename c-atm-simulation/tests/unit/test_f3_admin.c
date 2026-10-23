/**
 * tests/unit/test_f3_admin.c
 * Verification suite for F3 Administrator accounts, authentication, lockout, and roles.
 */

#include "admin.h"
#include "bank.h"
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
    printf("       RUNNING F3 ADMIN ACCOUNTS TEST SUITE         \n");
    printf("====================================================\n");

    Bank bank;
    bank_init(&bank);
    admin_seed_default(&bank);

    /* 1. Admin seeding verification */
    Admin *super = admin_find(&bank, "ADMIN001");
    TEST_CHECK(super != NULL, "Admin Seed: Default ADMIN001 seeded");
    TEST_CHECK(super && super->role == ROLE_SUPERADMIN, "Admin Seed: ADMIN001 is SUPERADMIN");
    TEST_CHECK(super && super->must_change_pass == true, "Admin Seed: must_change_pass is true on initial creation");

    /* 2. Authentication with default credentials */
    Admin *authed = NULL;
    char err[128];
    bool ok = admin_authenticate(&bank, "ADMIN001", "admin123", &authed, err, sizeof(err));
    TEST_CHECK(ok && authed == super, "Admin Auth: Valid default passphrase succeeds");

    /* 3. Passphrase change enforcement */
    bool changed = admin_change_passphrase(&bank, super, "SuperSecret2026!");
    TEST_CHECK(changed, "Admin Password: Change passphrase succeeds");
    TEST_CHECK(super->must_change_pass == false, "Admin Password: must_change_pass cleared after update");

    /* Old passphrase fails */
    ok = admin_authenticate(&bank, "ADMIN001", "admin123", &authed, err, sizeof(err));
    TEST_CHECK(!ok, "Admin Auth: Old passphrase no longer valid");

    /* New passphrase succeeds */
    ok = admin_authenticate(&bank, "ADMIN001", "SuperSecret2026!", &authed, err, sizeof(err));
    TEST_CHECK(ok, "Admin Auth: New passphrase authenticates successfully");

    /* 4. Superadmin creates an Auditor */
    bool created = admin_create_admin(&bank, "ADMIN001", "AUDIT001", "Jane Auditor", ROLE_AUDITOR, "auditpass1");
    TEST_CHECK(created, "Admin Management: SUPERADMIN can create new AUDITOR");
    Admin *auditor = admin_find(&bank, "AUDIT001");
    TEST_CHECK(auditor != NULL && auditor->role == ROLE_AUDITOR, "Admin Management: AUDITOR role preserved");

    /* Non-superadmin cannot create admin */
    bool fail_create = admin_create_admin(&bank, "AUDIT001", "ADMIN002", "Hacker Admin", ROLE_ADMIN, "hackerpass");
    TEST_CHECK(!fail_create, "Admin Role: AUDITOR cannot create other admin accounts");

    /* 5. Lockout after admin_max_tries failed attempts */
    for (int i = 0; i < bank.config.admin_max_tries; i++) {
        ok = admin_authenticate(&bank, "AUDIT001", "wrong_pass", &authed, err, sizeof(err));
        TEST_CHECK(!ok, "Admin Lockout: Incorrect passphrase rejected");
    }
    TEST_CHECK(auditor->is_locked == true, "Admin Lockout: Account locked after max failed tries");

    /* Locked account cannot log in even with correct password */
    ok = admin_authenticate(&bank, "AUDIT001", "auditpass1", &authed, err, sizeof(err));
    TEST_CHECK(!ok && strstr(err, "LOCKED") != NULL, "Admin Lockout: Locked account rejected with correct password");

    /* 6. Unlock by SUPERADMIN */
    bool unlocked = admin_unlock(&bank, "ADMIN001", "AUDIT001");
    TEST_CHECK(unlocked && auditor->is_locked == false, "Admin Unlock: SUPERADMIN successfully unlocks locked account");

    /* Login succeeds after unlock */
    ok = admin_authenticate(&bank, "AUDIT001", "auditpass1", &authed, err, sizeof(err));
    TEST_CHECK(ok, "Admin Auth: Unlocked admin can now authenticate");

    printf("\nSUMMARY: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail > 0 ? 1 : 0;
}
