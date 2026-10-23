/**
 * src/main_atm.c
 * Terminal ATM client entry point (Section 4.1).
 * Supports --standalone execution using local in-process BankApi.
 */

#include "bank.h"
#include "bank_api.h"
#include "menu_customer.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    bool standalone = true;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--standalone") == 0) {
            standalone = true;
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
        Bank bank;
        bank_init(&bank);
        bank.config = cfg;
        bank_seed_demo(&bank);

        BankApi api = bank_api_create_local(&bank);
        menu_customer_run(&api, &cfg);

        bank_destroy(&bank);
    } else {
        printf("[INFO] Remote daemon client scheduled for Step 4.\n");
    }

    return 0;
}
