/**
 * src/ui/menu_customer.c
 * Interactive terminal menu adhering strictly to Section 4.4 numbering.
 */

#include "menu_customer.h"
#include "input.h"
#include "money.h"
#include "render.h"
#include "clock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void handle_balance(BankApi *api, long account_id, const Config *cfg) {
    int64_t bal = 0;
    ApiResult res = api->get_balance(api->ctx, account_id, &bal);
    if (res.success) {
        char buf[32];
        money_format(bal, cfg->currency, buf, sizeof(buf));
        printf("\nCurrent Cleared Balance: %s\n", buf);
    } else {
        printf("Error: %s\n", res.message);
    }
}

static void handle_deposit(BankApi *api, long account_id, const Config *cfg) {
    printf("Enter deposit amount: ");
    char line[64];
    if (!input_get_line(line, sizeof(line))) return;
    int64_t minor = 0;
    if (!money_parse_minor(line, &minor) || minor <= 0) {
        printf("Invalid amount.\n");
        return;
    }
    ApiResult res = api->deposit(api->ctx, account_id, minor);
    if (res.success) {
        render_receipt(res.receipt_no, sim_day(), clock_now(), "DEPOSIT", minor, res.balance_after, cfg->currency, account_id, 0);
    } else {
        printf("Deposit failed: %s\n", res.message);
    }
}

static void handle_withdraw(BankApi *api, long account_id, const Config *cfg) {
    printf("Enter withdrawal amount (multiples of %lld): ", (long long)cfg->withdraw_multiple);
    long whole = 0;
    if (!input_get_long(&whole, 1, 1000000) || whole % cfg->withdraw_multiple != 0) {
        printf("Invalid amount. Must be a multiple of %lld.\n", (long long)cfg->withdraw_multiple);
        return;
    }
    int64_t minor = (int64_t)whole * 100;
    ApiResult res = api->withdraw(api->ctx, account_id, minor);
    if (res.success) {
        render_receipt(res.receipt_no, sim_day(), clock_now(), "WITHDRAWAL", minor, res.balance_after, cfg->currency, account_id, 0);
    } else {
        printf("Withdrawal failed: %s\n", res.message);
    }
}

static void handle_transfer(BankApi *api, long account_id, const Config *cfg) {
    printf("Enter destination account number: ");
    long dst = 0;
    if (!input_get_long(&dst, 1000, 999999)) {
        printf("Invalid destination account.\n");
        return;
    }
    printf("Enter transfer amount: ");
    char line[64];
    if (!input_get_line(line, sizeof(line))) return;
    int64_t minor = 0;
    if (!money_parse_minor(line, &minor) || minor <= 0) {
        printf("Invalid amount.\n");
        return;
    }
    ApiResult res = api->transfer(api->ctx, account_id, dst, minor, "");
    if (res.success) {
        render_receipt(res.receipt_no, sim_day(), clock_now(), "TRANSFER OUT", minor, res.balance_after, cfg->currency, account_id, dst);
    } else {
        printf("Transfer failed: %s\n", res.message);
    }
}

static void handle_history(BankApi *api, long account_id, const Config *cfg) {
    Txn txns[10];
    size_t count = 0;
    ApiResult res = api->get_history(api->ctx, account_id, txns, 5, &count);
    if (!res.success) {
        printf("Could not fetch history: %s\n", res.message);
        return;
    }
    printf("\n--- Last %zu Transactions ---\n", count);
    if (count == 0) {
        printf("No transaction records found.\n");
        return;
    }
    for (size_t i = 0; i < count; ++i) {
        char amt_buf[32], bal_buf[32];
        money_format(txns[i].amount, cfg->currency, amt_buf, sizeof(amt_buf));
        money_format(txns[i].balance_after, cfg->currency, bal_buf, sizeof(bal_buf));
        printf("%s: %s | Balance: %s | %s\n",
               txns[i].receipt_no, amt_buf, bal_buf, txns[i].note);
    }
}

static void handle_change_pin(BankApi *api, long account_id) {
    char old_pin[16], new_pin[16];
    printf("Enter current 4-digit PIN: ");
    if (!input_get_masked_pin(old_pin, sizeof(old_pin))) return;
    printf("Enter new 4-digit PIN: ");
    if (!input_get_masked_pin(new_pin, sizeof(new_pin))) return;

    ApiResult res = api->change_pin(api->ctx, account_id, old_pin, new_pin);
    if (res.success) {
        printf("PIN updated successfully.\n");
    } else {
        printf("Failed to change PIN: %s\n", res.message);
    }
}

void menu_customer_run(BankApi *api, const Config *cfg) {
    printf("Enter Account Number (0 to quit): ");
    long id = 0;
    if (!input_get_long(&id, 0, 999999) || id == 0) return;

    printf("Enter 4-digit PIN: ");
    char pin[16];
    if (!input_get_masked_pin(pin, sizeof(pin))) return;

    char holder[MAX_NAME_LEN + 1];
    ApiResult log_res = api->login(api->ctx, id, pin, holder);
    if (!log_res.success) {
        printf("Login error: %s\n", log_res.message);
        return;
    }

    printf("\n>>> Welcome, %s (Account: %ld) <<<\n", holder, id);
    bool in_session = true;
    while (in_session) {
        printf("\n================ CUSTOMER MENU ================\n");
        printf("1. Check Balance         7. Statements & Export\n");
        printf("2. Cash Deposit          8. Spending Analytics\n");
        printf("3. Cash Withdrawal       9. Standing Orders\n");
        printf("4. Transfer Funds       10. Loans (F5)\n");
        printf("5. Last 5 Transactions  11. Redeem Voucher (F6)\n");
        printf("6. Change PIN           12. Security Alerts (F2)\n");
        printf("0. Logout\n");
        printf("================================================\nChoice: ");

        int choice = -1;
        if (!input_get_int(&choice, 0, 12)) {
            printf("Invalid selection.\n");
            continue;
        }

        switch (choice) {
            case 1: handle_balance(api, id, cfg); break;
            case 2: handle_deposit(api, id, cfg); break;
            case 3: handle_withdraw(api, id, cfg); break;
            case 4: handle_transfer(api, id, cfg); break;
            case 5: handle_history(api, id, cfg); break;
            case 6: handle_change_pin(api, id); break;
            case 7: printf("[INFO] Statements module scheduled for Step 4.\n"); break;
            case 8: printf("[INFO] Analytics module scheduled for Step 4.\n"); break;
            case 9: printf("[INFO] Standing orders scheduled for Step 4.\n"); break;
            case 10: printf("[INFO] Loans module scheduled for Step 3.\n"); break;
            case 11: printf("[INFO] Vouchers module scheduled for Step 3.\n"); break;
            case 12: printf("[INFO] Security alerts scheduled for Step 3.\n"); break;
            case 0:
                api->logout(api->ctx, id);
                in_session = false;
                printf("Logged out cleanly.\n");
                break;
            default: break;
        }
    }
}
