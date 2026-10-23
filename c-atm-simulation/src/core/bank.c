/**
 * src/core/bank.c
 * Core bank lifecycle and local in-process BankApi implementation (Section 4.3).
 */

#include "bank.h"
#include "bank_api.h"
#include "account.h"
#include "txn.h"
#include "pin.h"
#include "clock.h"
#include "config.h"
#include "fraud.h"
#include "otp.h"
#include <stdio.h>
#include <string.h>

void bank_init(Bank *bank) {
    if (!bank) return;
    memset(bank, 0, sizeof(Bank));
    config_set_defaults(&bank->config);
    bank->sim_day = 0;
}

void bank_seed_demo(Bank *bank) {
    if (!bank) return;
    account_create(bank, 1001, "Alice Smith", 5000000LL, "1234");
    account_create(bank, 1002, "Bob Jones",   7500000LL, "2345");
    account_create(bank, 1003, "Charlie Day", 2500000LL, "3456");
}

void bank_destroy(Bank *bank) {
    if (!bank) return;
    memset(bank, 0, sizeof(Bank));
}

/* Local BankApi methods */

static ApiResult local_login(void *ctx, long account_id, const char *pin, char out_name[MAX_NAME_LEN + 1]) {
    Bank *b = (Bank *)ctx;
    ApiResult res = {false, 0, "", "", 0};
    Account *acc = account_find(b, account_id);
    if (!acc) {
        res.error_code = 404;
        snprintf(res.message, sizeof(res.message), "Account not found");
        return res;
    }
    if (acc->status == STATUS_BLOCKED) {
        res.error_code = 403;
        snprintf(res.message, sizeof(res.message), "Card is BLOCKED. Contact bank.");
        return res;
    }
    if (acc->status == STATUS_FROZEN) {
        res.error_code = 403;
        snprintf(res.message, sizeof(res.message), "Account is FROZEN. Contact bank.");
        return res;
    }

    if (!pin_verify(pin, acc->pin_salt, acc->pin_iter, acc->pin_hash)) {
        acc->failed_pins++;
        res.error_code = 401;
        snprintf(res.message, sizeof(res.message), "Incorrect PIN. Attempt %d of %d.",
                 acc->failed_pins, b->config.max_pin_tries);
        if (acc->failed_pins >= b->config.max_pin_tries) {
            acc->status = STATUS_BLOCKED;
            snprintf(res.message, sizeof(res.message), "Card is now BLOCKED after %d failed attempts.",
                     b->config.max_pin_tries);
        }
        return res;
    }

    acc->failed_pins = 0;
    res.success = true;
    res.balance_after = acc->balance;
    if (out_name) {
        snprintf(out_name, MAX_NAME_LEN + 1, "%s", acc->holder_name);
    }
    return res;
}

static ApiResult local_logout(void *ctx, long account_id) {
    (void)ctx;
    (void)account_id;
    ApiResult res = {true, 0, "Logged out successfully", "", 0};
    return res;
}

static ApiResult local_get_balance(void *ctx, long account_id, int64_t *out_balance) {
    Bank *b = (Bank *)ctx;
    ApiResult res = {false, 0, "", "", 0};
    Account *acc = account_find(b, account_id);
    if (!acc) {
        res.error_code = 404;
        snprintf(res.message, sizeof(res.message), "Account not found");
        return res;
    }
    if (out_balance) *out_balance = acc->balance;
    res.success = true;
    res.balance_after = acc->balance;
    return res;
}

static ApiResult local_deposit(void *ctx, long account_id, int64_t amount) {
    Bank *b = (Bank *)ctx;
    ApiResult res = {false, 0, "", "", 0};
    Account *acc = account_find(b, account_id);
    if (!acc || acc->status != STATUS_ACTIVE) {
        res.error_code = 400;
        snprintf(res.message, sizeof(res.message), "Invalid or inactive account");
        return res;
    }
    if (amount <= 0) {
        res.error_code = 400;
        snprintf(res.message, sizeof(res.message), "Invalid deposit amount");
        return res;
    }
    acc->balance += amount;
    txn_record(b, acc->id, TXN_DEPOSIT, amount, acc->balance, 0, "ATM Cash Deposit", res.receipt_no);
    res.success = true;
    res.balance_after = acc->balance;
    snprintf(res.message, sizeof(res.message), "Deposit successful");
    return res;
}

static ApiResult local_withdraw(void *ctx, long account_id, int64_t amount) {
    Bank *b = (Bank *)ctx;
    ApiResult res = {false, 0, "", "", 0};
    Account *acc = account_find(b, account_id);
    if (!acc) {
        res.error_code = 404;
        snprintf(res.message, sizeof(res.message), "Account not found");
        return res;
    }
    if (acc->status != STATUS_ACTIVE) {
        res.error_code = 403;
        snprintf(res.message, sizeof(res.message), "Account not active (status: %s)",
                 acc->status == STATUS_FROZEN ? "FROZEN" : "BLOCKED");
        return res;
    }

    account_reset_daily_limits_if_needed(acc, sim_day());
    char reason[64];
    if (!account_can_withdraw(b, acc, amount, reason, sizeof(reason))) {
        res.error_code = 400;
        snprintf(res.message, sizeof(res.message), "%s", reason);
        return res;
    }

    acc->balance -= amount;
    acc->withdrawn_today += amount;
    txn_record(b, acc->id, TXN_WITHDRAW, amount, acc->balance, 0, "ATM Cash Withdrawal", res.receipt_no);
    fraud_eval_operation(b, acc, TXN_WITHDRAW, amount, 0);

    res.success = true;
    res.balance_after = acc->balance;
    snprintf(res.message, sizeof(res.message), "Withdrawal successful");
    return res;
}

static ApiResult local_transfer(void *ctx, long src_id, long dst_id, int64_t amount, const char *otp) {
    Bank *b = (Bank *)ctx;
    ApiResult res = {false, 0, "", "", 0};
    if (src_id == dst_id) {
        res.error_code = 400;
        snprintf(res.message, sizeof(res.message), "Cannot transfer to same account");
        return res;
    }
    Account *src = account_find(b, src_id);
    Account *dst = account_find(b, dst_id);
    if (!src || !dst || src->status != STATUS_ACTIVE || dst->status != STATUS_ACTIVE) {
        res.error_code = 400;
        snprintf(res.message, sizeof(res.message), "Sender or recipient account not active");
        return res;
    }
    if (amount <= 0 || src->balance < amount) {
        res.error_code = 400;
        snprintf(res.message, sizeof(res.message), "Insufficient funds");
        return res;
    }

    if (otp && strlen(otp) == 6) {
        char err[128];
        if (!otp_verify(b, src_id, OTP_PURPOSE_TRANSFER, amount, otp, err, sizeof(err))) {
            res.error_code = 401;
            snprintf(res.message, sizeof(res.message), "Verification error: %.100s", err);
            return res;
        }
    }

    src->balance -= amount;
    dst->balance += amount;
    txn_record(b, src->id, TXN_TRANSFER_OUT, amount, src->balance, dst->id, "Fund Transfer Out", res.receipt_no);
    char dummy_rec[RECEIPT_ID_LEN + 1];
    txn_record(b, dst->id, TXN_TRANSFER_IN, amount, dst->balance, src->id, "Fund Transfer In", dummy_rec);
    fraud_eval_operation(b, src, TXN_TRANSFER_OUT, amount, dst->id);

    res.success = true;
    res.balance_after = src->balance;
    snprintf(res.message, sizeof(res.message), "Transfer successful");
    return res;
}

static ApiResult local_get_history(void *ctx, long account_id, Txn *out_txns, size_t max_count, size_t *out_count) {
    Bank *b = (Bank *)ctx;
    ApiResult res = {false, 0, "", "", 0};
    size_t count = txn_get_history(b, account_id, out_txns, max_count);
    if (out_count) *out_count = count;
    res.success = true;
    return res;
}

static ApiResult local_change_pin(void *ctx, long account_id, const char *old_pin, const char *new_pin) {
    Bank *b = (Bank *)ctx;
    ApiResult res = {false, 0, "", "", 0};
    Account *acc = account_find(b, account_id);
    if (!acc) {
        res.error_code = 404;
        snprintf(res.message, sizeof(res.message), "Account not found");
        return res;
    }
    if (old_pin && !pin_verify(old_pin, acc->pin_salt, acc->pin_iter, acc->pin_hash)) {
        res.error_code = 401;
        snprintf(res.message, sizeof(res.message), "Current PIN is incorrect");
        return res;
    }
    if (strlen(new_pin) != 4) {
        res.error_code = 400;
        snprintf(res.message, sizeof(res.message), "PIN must be exactly 4 digits");
        return res;
    }

    pin_generate_salt(acc->pin_salt);
    pin_hash_derive(new_pin, acc->pin_salt, acc->pin_iter, acc->pin_hash);
    acc->must_change_pin = false;
    res.success = true;
    snprintf(res.message, sizeof(res.message), "PIN changed successfully");
    return res;
}

BankApi bank_api_create_local(Bank *bank) {
    BankApi api;
    api.ctx = bank;
    api.login = local_login;
    api.logout = local_logout;
    api.get_balance = local_get_balance;
    api.deposit = local_deposit;
    api.withdraw = local_withdraw;
    api.transfer = local_transfer;
    api.get_history = local_get_history;
    api.change_pin = local_change_pin;
    return api;
}
