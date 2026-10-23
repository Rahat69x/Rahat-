/**
 * src/main_bankd.c
 * Central bank daemon process (F8). The only process that owns and persists banking data.
 */

#include "types.h"
#include "bank.h"
#include "config.h"
#include "store.h"
#include "audit.h"
#include "server.h"
#include "scheduler.h"
#include "clock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static volatile int g_stop = 0;
static Bank g_bank;
static BankServer g_server;

static void handle_sig(int sig) {
    (void)sig;
    g_stop = 1;
}

int main(int argc, char **argv) {
    printf("============================================================\n");
    printf("      C ATM SIMULATION - CENTRAL BANK DAEMON (bankd)        \n");
    printf("============================================================\n");

    bank_init(&g_bank);
    config_load("config.ini", &g_bank.config);

    const char *data_dir = "data";
    int auto_day_sec = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--data-dir") == 0 && i + 1 < argc) {
            data_dir = argv[++i];
        } else if (strcmp(argv[i], "--auto-day-seconds") == 0 && i + 1 < argc) {
            auto_day_sec = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--reset-demo") == 0) {
            bank_seed_demo(&g_bank);
            store_save_bank(data_dir, &g_bank);
            printf("[INFO] Demo accounts seeded and saved.\n");
        }
    }

    /* Single writer lock */
    char lock_err[128];
    if (!store_acquire_lock(data_dir, lock_err, sizeof(lock_err))) {
        fprintf(stderr, "[FATAL] %s\n", lock_err);
        return 1;
    }

    if (!store_load_bank(data_dir, &g_bank)) {
        printf("[INFO] No existing database found. Seeding demo accounts...\n");
        bank_seed_demo(&g_bank);
        store_save_bank(data_dir, &g_bank);
    }
    audit_init(&g_bank);

    if (strcmp(g_bank.config.server_bind, "127.0.0.1") != 0 &&
        strcmp(g_bank.config.server_bind, "localhost") != 0) {
        printf("[WARN] Non-localhost bind address detected: %s (No TLS; educational use only!)\n",
               g_bank.config.server_bind);
    }

    signal(SIGINT, handle_sig);
    signal(SIGTERM, handle_sig);

    if (!server_init(&g_server, &g_bank, g_bank.config.server_bind,
                    g_bank.config.server_port, g_bank.config.server_max_clients)) {
        fprintf(stderr, "[FATAL] Failed to bind server socket on %s:%d\n",
                g_bank.config.server_bind, g_bank.config.server_port);
        store_release_lock(data_dir);
        return 1;
    }

    printf("[INFO] bankd listening on %s:%d (Max clients: %d)\n",
           g_bank.config.server_bind, g_bank.config.server_port, g_bank.config.server_max_clients);
    audit_record(&g_bank, data_dir, "SYSTEM", "SERVER_START", "-", "bankd started");

    time_t last_day_tick = clock_now();
    while (!g_stop) {
        server_run(&g_server, &g_stop);
        if (auto_day_sec > 0 && clock_now() - last_day_tick >= auto_day_sec) {
            scheduler_advance_days(&g_bank, 1);
            store_save_bank(data_dir, &g_bank);
            last_day_tick = clock_now();
        }
    }

    printf("\n[INFO] Shutting down bankd gracefully...\n");
    server_close(&g_server);
    store_save_bank(data_dir, &g_bank);
    audit_record(&g_bank, data_dir, "SYSTEM", "SERVER_SHUTDOWN", "-", "bankd clean shutdown");
    store_release_lock(data_dir);
    printf("[INFO] Server stopped. All data persisted.\n");
    return 0;
}
