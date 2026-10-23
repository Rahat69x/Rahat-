/**
 * @file main.c
 * @brief Main Entry Point for Console-based ATM Simulation (C11 Standard)
 * Clean orchestration: configuration, persistence, recovery, and execution loop.
 */

#include "atm.h"

int main(void) {
    /* Initialize pseudorandom seed for OTP and salt generation */
    srand((unsigned int)time(NULL));

    /* Initialize runtime context */
    AtmContext atm;
    memset(&atm, 0, sizeof(AtmContext));
    atm.operational_status = ATM_OPERATIONAL;
    atm.cash.notes_1000 = 100; /* 100,000 BDT */
    atm.cash.notes_500 = 200;  /* 100,000 BDT */
    atm.cash.notes_100 = 500;  /*  50,000 BDT */
    atm.dispute_count = 0;
    atm.last_activity_time = time(NULL);

    /* 1. Load banking rules configuration (A19) */
    load_config_file(CONFIG_FILE, &atm.config);

    /* 2. Load foreign exchange rates table (A6) */
    load_rates_file(RATES_FILE, &atm.rates);

    /* 3. Load or recover accounts database (Core, A18) */
    printf("================================================================================\n");
    printf("           ANTIGRAVITY CONSOLE ATM SIMULATION (C11 STANDARD)                    \n");
    printf("================================================================================\n");
    printf("[*] Verifying database integrity...\n");

    if (!load_all_data(ACCOUNTS_FILE)) {
        /* Primary file missing or corrupted: check backup */
        if (!recover_from_backup()) {
            /* Both primary and backup missing: initialize demo accounts */
            init_demo_accounts();
        }
    } else {
        printf("[+] Verified: Primary database loaded with integrity validation (A18).\n");
    }

    audit_log("SYSTEM_START", 0, "ATM terminal operational session started");

    /* 4. Launch terminal interaction loop */
    main_atm_loop(&atm);

    /* 5. Safe shutdown: flush persistence and release dynamic memory */
    save_all_data(ACCOUNTS_FILE);
    cleanup_system_memory();

    return 0;
}
