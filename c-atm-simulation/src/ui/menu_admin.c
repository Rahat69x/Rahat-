/**
 * src/ui/menu_admin.c
 * Administrator terminal menu interface (F3).
 */

#include "menu_admin.h"
#include "admin.h"
#include "audit.h"
#include "input.h"
#include "account.h"
#include "render.h"
#include "money.h"
#include "store.h"
#include "pin.h"
#include <stdio.h>
#include <string.h>

void menu_admin_run(Bank *bank, const char *data_dir) {
    printf("\n=== ADMINISTRATOR ACCESS ===\n");
    printf("Admin ID: ");
    char id[16];
    if (!input_get_line(id, sizeof(id))) return;

    printf("Passphrase: ");
    char pass[64];
    if (!input_get_line(pass, sizeof(pass))) return;

    Admin *adm = NULL;
    char err[128];
    if (!admin_authenticate(bank, id, pass, &adm, err, sizeof(err))) {
        printf("Authentication failed: %s\n", err);
        audit_record(bank, data_dir, id, "ADMIN_LOGIN_FAIL", "-", err);
        return;
    }

    audit_record(bank, data_dir, adm->id, "ADMIN_LOGIN_OK", "-", "Administrator logged in");

    if (adm->must_change_pass) {
        printf("\n[MANDATORY SECURITY NOTICE] You must change your temporary passphrase.\n");
        printf("Enter new passphrase (min 6 chars): ");
        char new_pass[64];
        if (input_get_line(new_pass, sizeof(new_pass)) && strlen(new_pass) >= 6) {
            admin_change_passphrase(bank, adm, new_pass);
            printf("Passphrase updated successfully.\n");
            audit_record(bank, data_dir, adm->id, "ADMIN_PASS_CHANGE", adm->id, "Initial passphrase rotated");
            store_save_bank(data_dir, bank);
        } else {
            printf("Password rotation cancelled or too short. Logging out.\n");
            return;
        }
    }

    bool in_menu = true;
    while (in_menu) {
        printf("\n================ ADMINISTRATOR CONSOLE [%s] ================\n",
               adm->role == ROLE_SUPERADMIN ? "SUPERADMIN" :
               adm->role == ROLE_ADMIN ? "ADMIN" : "AUDITOR");
        printf("1. List Accounts (Masked)      6. Unblock / Unfreeze Card\n");
        printf("2. Create New Account          7. Reset Customer PIN (Temp)\n");
        printf("3. Close Account               8. Verify Audit Chain (F7)\n");
        printf("4. View Alerts (F2)            9. Manage Admins (Superadmin)\n");
        printf("5. Set Account Limits          0. Logout\n");
        printf("============================================================\nChoice: ");

        int choice = -1;
        if (!input_get_int(&choice, 0, 9)) continue;

        switch (choice) {
            case 1: {
                printf("\n%-8s %-20s %-16s %-10s\n", "Acc ID", "Holder Name", "Balance", "Status");
                printf("----------------------------------------------------------\n");
                for (size_t i = 0; i < bank->account_count; ++i) {
                    Account *a = &bank->accounts[i];
                    char bal_buf[32], masked_acc[32];
                    money_format(a->balance, bank->config.currency, bal_buf, sizeof(bal_buf));
                    render_mask_account(a->id, masked_acc, sizeof(masked_acc));
                    printf("%-8s %-20s %-16s %-10s\n",
                           masked_acc, a->holder_name, bal_buf,
                           a->status == STATUS_ACTIVE ? "ACTIVE" :
                           a->status == STATUS_BLOCKED ? "BLOCKED" : "FROZEN");
                }
                break;
            }
            case 2: {
                if (adm->role == ROLE_AUDITOR) { printf("Permission denied: Read-only role.\n"); break; }
                printf("Enter new Account ID (1000-999999): ");
                long nid = 0;
                if (!input_get_long(&nid, 1000, 999999)) break;
                printf("Enter Customer Legal Name: ");
                char name[MAX_NAME_LEN + 1];
                if (!input_get_line(name, sizeof(name))) break;
                printf("Enter initial deposit amount: ");
                char amt_line[64];
                if (!input_get_line(amt_line, sizeof(amt_line))) break;
                int64_t init_bal = 0;
                if (!money_parse_minor(amt_line, &init_bal)) break;
                printf("Set initial 4-digit PIN: ");
                char pin[16];
                if (!input_get_masked_pin(pin, sizeof(pin))) break;

                Account *created = account_create(bank, nid, name, init_bal, pin);
                if (created) {
                    printf("Account %ld created successfully!\n", nid);
                    audit_record(bank, data_dir, adm->id, "ACCOUNT_CREATE", name, "New account registered");
                    store_save_bank(data_dir, bank);
                } else {
                    printf("Failed to create account (ID exists or memory full).\n");
                }
                break;
            }
            case 6: {
                if (adm->role == ROLE_AUDITOR) { printf("Permission denied: Read-only role.\n"); break; }
                printf("Enter Account ID to Unblock/Unfreeze: ");
                long tid = 0;
                if (!input_get_long(&tid, 1000, 999999)) break;
                Account *target = account_find(bank, tid);
                if (target) {
                    target->status = STATUS_ACTIVE;
                    target->failed_pins = 0;
                    printf("Account %ld is now ACTIVE.\n", tid);
                    audit_record(bank, data_dir, adm->id, "CARD_UNBLOCK", "ACC", "Card unblocked by admin");
                    store_save_bank(data_dir, bank);
                } else {
                    printf("Account not found.\n");
                }
                break;
            }
            case 7: {
                if (adm->role == ROLE_AUDITOR) { printf("Permission denied: Read-only role.\n"); break; }
                printf("Enter Account ID to Reset PIN: ");
                long tid = 0;
                if (!input_get_long(&tid, 1000, 999999)) break;
                Account *target = account_find(bank, tid);
                if (target) {
                    char temp_pin[5] = "4321";
                    pin_generate_salt(target->pin_salt);
                    pin_hash_derive(temp_pin, target->pin_salt, target->pin_iter, target->pin_hash);
                    target->must_change_pin = true;
                    target->failed_pins = 0;
                    printf("Temporary PIN issued: %s (customer will be prompted to change on login)\n", temp_pin);
                    audit_record(bank, data_dir, adm->id, "PIN_RESET", "ACC", "Temporary PIN assigned");
                    store_save_bank(data_dir, bank);
                } else {
                    printf("Account not found.\n");
                }
                break;
            }
            case 8: {
                printf("\n[*] Verifying tamper-evident audit log chain...\n");
                uint64_t broken = 0;
                char rsn[128];
                if (audit_verify(data_dir, bank->audit_head_hash, &broken, rsn, sizeof(rsn))) {
                    printf("[+] Audit log integrity verified: 100%% authentic hash chain!\n");
                } else {
                    printf("[-] AUDIT INTEGRITY FAILURE at sequence %llu: %s\n", (unsigned long long)broken, rsn);
                }
                break;
            }
            case 0:
                in_menu = false;
                printf("Administrator logged out. Current Head Hash: ");
                for (int i = 0; i < 8; ++i) printf("%02x", bank->audit_head_hash[i]);
                printf("...\n");
                audit_record(bank, data_dir, adm->id, "ADMIN_LOGOUT", "-", "Admin session ended");
                break;
            default:
                printf("Feature action recorded for later steps.\n");
                break;
        }
    }
}
