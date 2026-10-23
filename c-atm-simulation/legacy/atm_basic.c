/**
 * legacy/atm_basic.c
 * Baseline terminal ATM in standard C11.
 * In-memory storage, 3 demo accounts, PIN authentication, 3-attempt lockout.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_TXNS 50
#define PER_WITHDRAW_LIMIT 20000LL
#define DAILY_WITHDRAW_LIMIT 50000LL

typedef enum {
    BASIC_DEPOSIT,
    BASIC_WITHDRAW,
    BASIC_TRANSFER_OUT,
    BASIC_TRANSFER_IN
} BasicTxnType;

typedef struct {
    BasicTxnType type;
    int64_t amount;
    int64_t balance_after;
    long counterparty;
} BasicTxn;

typedef struct {
    long id;
    char name[32];
    char pin[5];
    int64_t balance;
    int64_t withdrawn_today;
    bool is_blocked;
    int failed_pins;
    BasicTxn history[MAX_TXNS];
    int history_count;
} BasicAccount;

static BasicAccount g_accounts[3] = {
    {1001, "Alice Smith", "1234", 5000000LL, 0, false, 0, {{0}}, 0},
    {1002, "Bob Jones",   "2345", 7500000LL, 0, false, 0, {{0}}, 0},
    {1003, "Charlie Day", "3456", 2500000LL, 0, false, 0, {{0}}, 0}
};

static BasicAccount *find_account(long id) {
    for (int i = 0; i < 3; i++) {
        if (g_accounts[i].id == id) return &g_accounts[i];
    }
    return NULL;
}

static void record_txn(BasicAccount *acc, BasicTxnType type, int64_t amount, long counterparty) {
    if (acc->history_count < MAX_TXNS) {
        acc->history[acc->history_count].type = type;
        acc->history[acc->history_count].amount = amount;
        acc->history[acc->history_count].balance_after = acc->balance;
        acc->history[acc->history_count].counterparty = counterparty;
        acc->history_count++;
    }
}

static void show_history(BasicAccount *acc) {
    printf("\n--- Last Transactions (Max 5) ---\n");
    int start = acc->history_count > 5 ? acc->history_count - 5 : 0;
    if (acc->history_count == 0) {
        printf("No transactions yet.\n");
        return;
    }
    for (int i = acc->history_count - 1; i >= start; i--) {
        const char *t = "TXN";
        switch (acc->history[i].type) {
            case BASIC_DEPOSIT: t = "DEPOSIT"; break;
            case BASIC_WITHDRAW: t = "WITHDRAW"; break;
            case BASIC_TRANSFER_OUT: t = "TRANSFER OUT"; break;
            case BASIC_TRANSFER_IN: t = "TRANSFER IN"; break;
        }
        printf("%s: $%.2f | Balance: $%.2f\n",
               t, (double)acc->history[i].amount / 100.0,
               (double)acc->history[i].balance_after / 100.0);
    }
}

int main(void) {
    printf("=== LEGACY ATM BASELINE ===\n");
    char line[128];
    while (1) {
        printf("\nEnter Account Number (0 to exit): ");
        if (!fgets(line, sizeof(line), stdin)) break;
        long id = strtol(line, NULL, 10);
        if (id == 0) break;

        BasicAccount *acc = find_account(id);
        if (!acc) {
            printf("Account not found.\n");
            continue;
        }
        if (acc->is_blocked) {
            printf("Card is BLOCKED. Contact bank.\n");
            continue;
        }

        printf("Enter 4-digit PIN: ");
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\r\n")] = '\0';

        if (strcmp(acc->pin, line) != 0) {
            acc->failed_pins++;
            printf("Incorrect PIN. Attempt %d of 3.\n", acc->failed_pins);
            if (acc->failed_pins >= 3) {
                acc->is_blocked = true;
                printf("Card is now BLOCKED.\n");
            }
            continue;
        }

        acc->failed_pins = 0;
        printf("\nWelcome, %s!\n", acc->name);

        int logged_in = 1;
        while (logged_in) {
            printf("\n1. Check Balance\n2. Deposit\n3. Withdraw\n4. Transfer\n5. Last 5 Txns\n6. Change PIN\n0. Logout\nChoice: ");
            if (!fgets(line, sizeof(line), stdin)) break;
            int choice = (int)strtol(line, NULL, 10);

            switch (choice) {
                case 1:
                    printf("Balance: $%.2f\n", (double)acc->balance / 100.0);
                    break;
                case 2: {
                    printf("Deposit amount ($): ");
                    if (!fgets(line, sizeof(line), stdin)) break;
                    double amt_d = strtod(line, NULL);
                    int64_t amt = (int64_t)(amt_d * 100.0 + 0.5);
                    if (amt <= 0) { printf("Invalid amount.\n"); break; }
                    acc->balance += amt;
                    record_txn(acc, BASIC_DEPOSIT, amt, 0);
                    printf("Deposited $%.2f. New Balance: $%.2f\n", amt_d, (double)acc->balance / 100.0);
                    break;
                }
                case 3: {
                    printf("Withdraw amount ($ in multiples of 100): ");
                    if (!fgets(line, sizeof(line), stdin)) break;
                    long whole = strtol(line, NULL, 10);
                    if (whole <= 0 || whole % 100 != 0) {
                        printf("Must be a positive multiple of 100.\n");
                        break;
                    }
                    int64_t amt = (int64_t)whole * 100;
                    if (whole > PER_WITHDRAW_LIMIT) {
                        printf("Exceeds per-withdrawal limit of $%ld.\n", (long)PER_WITHDRAW_LIMIT);
                        break;
                    }
                    if (acc->withdrawn_today + whole > DAILY_WITHDRAW_LIMIT) {
                        printf("Exceeds daily withdrawal limit of $%ld.\n", (long)DAILY_WITHDRAW_LIMIT);
                        break;
                    }
                    if (acc->balance < amt) {
                        printf("Insufficient funds.\n");
                        break;
                    }
                    acc->balance -= amt;
                    acc->withdrawn_today += whole;
                    record_txn(acc, BASIC_WITHDRAW, amt, 0);
                    printf("Withdrawn $%ld. New Balance: $%.2f\n", whole, (double)acc->balance / 100.0);
                    break;
                }
                case 4: {
                    printf("Recipient Account Number: ");
                    if (!fgets(line, sizeof(line), stdin)) break;
                    long target_id = strtol(line, NULL, 10);
                    BasicAccount *target = find_account(target_id);
                    if (!target || target->id == acc->id) {
                        printf("Invalid recipient.\n");
                        break;
                    }
                    printf("Transfer to %s. Amount ($): ", target->name);
                    if (!fgets(line, sizeof(line), stdin)) break;
                    double amt_d = strtod(line, NULL);
                    int64_t amt = (int64_t)(amt_d * 100.0 + 0.5);
                    if (amt <= 0 || acc->balance < amt) {
                        printf("Invalid or insufficient funds.\n");
                        break;
                    }
                    acc->balance -= amt;
                    target->balance += amt;
                    record_txn(acc, BASIC_TRANSFER_OUT, amt, target->id);
                    record_txn(target, BASIC_TRANSFER_IN, amt, acc->id);
                    printf("Transferred $%.2f to %s. New Balance: $%.2f\n", amt_d, target->name, (double)acc->balance / 100.0);
                    break;
                }
                case 5:
                    show_history(acc);
                    break;
                case 6: {
                    printf("Enter new 4-digit PIN: ");
                    if (!fgets(line, sizeof(line), stdin)) break;
                    line[strcspn(line, "\r\n")] = '\0';
                    if (strlen(line) != 4) {
                        printf("PIN must be exactly 4 digits.\n");
                        break;
                    }
                    memcpy(acc->pin, line, 5);
                    printf("PIN updated successfully.\n");
                    break;
                }
                case 0:
                    logged_in = 0;
                    printf("Logged out.\n");
                    break;
                default:
                    printf("Invalid option.\n");
                    break;
            }
        }
    }
    return 0;
}
