/**
 * src/main_atm.c
 * Terminal ATM client entry point (Section 4.1).
 * Supports --standalone execution and remote TCP connection to bankd.
 */

#include "bank.h"
#include "bank_api.h"
#include "client.h"
#include "menu_customer.h"
#include "config.h"
#include "store.h"
#include "audit.h"
#include <stdio.h>
#include <string.h>

static Bank g_bank;

int main(int argc, char **argv) {
    bool standalone = false;
    bool verify_audit = false;
    const char *data_dir = "data";

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--standalone") == 0) {
            standalone = true;
        } else if (strcmp(argv[i], "--verify-audit") == 0) {
            verify_audit = true;
        } else if (strcmp(argv[i], "--data-dir") == 0 && i + 1 < argc) {
            data_dir = argv[++i];
        }
    }

    /* Handle audit verification flag */
    if (verify_audit) {
        bank_init(&g_bank);
        store_load_bank(data_dir, &g_bank);
        uint64_t broken = 0;
        char reason[128];
        bool ok = audit_verify(data_dir, g_bank.audit_head_hash, &broken, reason, sizeof(reason));
        if (ok) {
            printf("[VERIFY PASS] Cryptographic audit log hash chain verified intact.\n");
            return 0;
        } else {
            printf("[VERIFY FAIL] Audit chain broken at sequence %llu: %s\n", (unsigned long long)broken, reason);
            return 1;
        }
    }

    printf("====================================================\n");
    printf("   c-atm-simulation Terminal Client (ISO C11)      \n");
    printf("   Educational Simulation - No Real Banking Data    \n");
    printf("====================================================\n");

    Config cfg;
    if (!config_load("config.ini", &cfg)) {
        printf("[WARN] Could not load config.ini; using defaults.\n");
    }

    if (standalone) {
        printf("[INFO] Running in STANDALONE mode (in-process core).\n");
        bank_init(&g_bank);
        g_bank.config = cfg;
        if (!store_load_bank(data_dir, &g_bank)) {
            bank_seed_demo(&g_bank);
            store_save_bank(data_dir, &g_bank);
        }
        audit_init(&g_bank);

        BankApi api = bank_api_create_local(&g_bank);
        menu_customer_run(&api, &cfg);

        store_save_bank(data_dir, &g_bank);
        bank_destroy(&g_bank);
    } else {
        printf("[INFO] Connecting to remote bankd on %s:%d...\n", cfg.server_bind, cfg.server_port);
        BankApi api = bank_api_create_remote(cfg.server_bind, cfg.server_port);
        menu_customer_run(&api, &cfg);
    }

    return 0;
}
