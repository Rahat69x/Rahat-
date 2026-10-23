/**
 * src/core/account.c
 * Account domain model operations (Section 4.2).
 */

#include "account.h"
#include "pin.h"
#include <stdio.h>
#include <string.h>

Account *account_find(Bank *bank, long id) {
    if (!bank) return NULL;
    for (size_t i = 0; i < bank->account_count; ++i) {
        if (bank->accounts[i].id == id) {
            return &bank->accounts[i];
        }
    }
    return NULL;
}

Account *account_create(Bank *bank, long id, const char *name, int64_t initial_balance, const char *pin) {
    if (!bank || bank->account_count >= MAX_ACCOUNTS) return NULL;
    if (account_find(bank, id) != NULL) return NULL;

    Account *acc = &bank->accounts[bank->account_count++];
    memset(acc, 0, sizeof(Account));
    acc->id = id;
    snprintf(acc->holder_name, sizeof(acc->holder_name), "%s", name);
    acc->balance = initial_balance;
    acc->status = STATUS_ACTIVE;
    acc->pin_iter = bank->config.pin_hash_iterations ? bank->config.pin_hash_iterations : 10000;
    
    pin_generate_salt(acc->pin_salt);
    pin_hash_derive(pin, acc->pin_salt, acc->pin_iter, acc->pin_hash);
    return acc;
}

void account_reset_daily_limits_if_needed(Account *acc, int current_day) {
    if (!acc) return;
    if (acc->last_withdraw_day != current_day) {
        acc->withdrawn_today = 0;
        acc->last_withdraw_day = current_day;
    }
}

bool account_can_withdraw(const Bank *bank, const Account *acc, int64_t amount, char *reason, size_t reason_len) {
    if (!bank || !acc || !reason || reason_len == 0) return false;

    if (acc->status != STATUS_ACTIVE) {
        snprintf(reason, reason_len, "Account is not active");
        return false;
    }
    if (amount <= 0) {
        snprintf(reason, reason_len, "Amount must be strictly positive");
        return false;
    }
    int64_t mult = bank->config.withdraw_multiple ? bank->config.withdraw_multiple * 100 : 10000;
    if ((amount % mult) != 0) {
        snprintf(reason, reason_len, "Withdrawal must be a multiple of %lld", (long long)(mult / 100));
        return false;
    }
    int64_t per_lim = acc->per_withdraw_override > 0 ? acc->per_withdraw_override : bank->config.per_withdraw_limit * 100;
    if (amount > per_lim) {
        snprintf(reason, reason_len, "Exceeds per-withdrawal limit");
        return false;
    }
    int64_t daily_lim = acc->daily_withdraw_override > 0 ? acc->daily_withdraw_override : bank->config.daily_withdraw_limit * 100;
    if (acc->withdrawn_today + amount > daily_lim) {
        snprintf(reason, reason_len, "Exceeds daily withdrawal limit");
        return false;
    }
    if (acc->balance < amount) {
        snprintf(reason, reason_len, "Insufficient account balance");
        return false;
    }
    return true;
}
