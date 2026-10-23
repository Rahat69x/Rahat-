/**
 * src/features/standing.c
 * Customer standing order creation, lifecycle, and automated execution (F9).
 */

#include "standing.h"
#include "account.h"
#include "txn.h"
#include "audit.h"
#include "fraud.h"
#include <stdio.h>
#include <string.h>

bool standing_create(Bank *bank, long account_id, long dest_id, int64_t amount,
                     Frequency freq, int start_day, int count, uint64_t *out_id,
                     char *err_msg, size_t err_len) {
    if (!bank) return false;
    if (account_id == dest_id) {
        if (err_msg) snprintf(err_msg, err_len, "Cannot create standing order to same account");
        return false;
    }
    Account *src = account_find(bank, account_id);
    Account *dst = account_find(bank, dest_id);
    if (!src || !dst || src->status != STATUS_ACTIVE || dst->status != STATUS_ACTIVE) {
        if (err_msg) snprintf(err_msg, err_len, "Invalid sender or recipient account");
        return false;
    }
    if (amount <= 0) {
        if (err_msg) snprintf(err_msg, err_len, "Amount must be strictly positive");
        return false;
    }

    int active_so = 0;
    for (size_t i = 0; i < bank->standing_order_count; ++i) {
        if (bank->standing_orders[i].account_id == account_id && bank->standing_orders[i].is_active) {
            active_so++;
        }
    }
    if (active_so >= 5) {
        if (err_msg) snprintf(err_msg, err_len, "Maximum 5 standing orders allowed per account");
        return false;
    }

    if (bank->standing_order_count >= MAX_STANDING_ORDERS) {
        if (err_msg) snprintf(err_msg, err_len, "Bank standing order capacity reached");
        return false;
    }

    StandingOrder *so = &bank->standing_orders[bank->standing_order_count++];
    memset(so, 0, sizeof(StandingOrder));
    so->id = ++bank->next_so_id;
    so->account_id = account_id;
    so->dest_account_id = dest_id;
    so->amount = amount;
    so->frequency = freq;
    so->start_day = (start_day > 0) ? start_day : bank->sim_day;
    so->remaining_count = count;
    so->last_run_day = 0;
    so->consecutive_failures = 0;
    so->is_active = true;

    if (out_id) *out_id = so->id;

    audit_record(bank, "data", "CUSTOMER", "SO_CREATE", "-", "Created recurring standing order");
    return true;
}

bool standing_cancel(Bank *bank, long account_id, uint64_t order_id) {
    if (!bank) return false;
    for (size_t i = 0; i < bank->standing_order_count; ++i) {
        StandingOrder *so = &bank->standing_orders[i];
        if (so->id == order_id && so->account_id == account_id) {
            so->is_active = false;
            audit_record(bank, "data", "CUSTOMER", "SO_CANCEL", "-", "Cancelled standing order");
            return true;
        }
    }
    return false;
}

void standing_execute_due(Bank *bank) {
    if (!bank) return;
    for (size_t i = 0; i < bank->standing_order_count; ++i) {
        StandingOrder *so = &bank->standing_orders[i];
        if (!so->is_active || bank->sim_day < so->start_day || so->last_run_day == bank->sim_day) {
            continue;
        }

        int days_elapsed = bank->sim_day - so->start_day;
        if (days_elapsed % (int)so->frequency != 0) {
            continue;
        }

        Account *src = account_find(bank, so->account_id);
        Account *dst = account_find(bank, so->dest_account_id);
        if (!src || !dst || src->status != STATUS_ACTIVE || dst->status != STATUS_ACTIVE || src->balance < so->amount) {
            so->consecutive_failures++;
            char msg[96];
            snprintf(msg, sizeof(msg), "Standing order #%llu execution failed (attempt %d/3)",
                     (unsigned long long)so->id, so->consecutive_failures);
            if (so->consecutive_failures >= 3) {
                so->is_active = false;
                fraud_add_alert(bank, so->account_id, 0, src ? src->risk_score : 0,
                                "Standing order suspended due to 3 consecutive failures");
                audit_record(bank, "data", "SYSTEM", "SO_SUSPENDED", "-", msg);
            }
            continue;
        }

        /* Execute recurring transfer */
        src->balance -= so->amount;
        dst->balance += so->amount;
        txn_record(bank, src->id, TXN_STANDING_ORDER, so->amount, src->balance, dst->id, "Standing Order Transfer Out", NULL);
        txn_record(bank, dst->id, TXN_STANDING_ORDER, so->amount, dst->balance, src->id, "Standing Order Transfer In", NULL);

        so->last_run_day = bank->sim_day;
        so->consecutive_failures = 0;
        if (so->remaining_count > 0) {
            so->remaining_count--;
            if (so->remaining_count == 0) {
                so->is_active = false;
            }
        }
        audit_record(bank, "data", "SYSTEM", "SO_EXECUTE", "-", "Standing order debited successfully");
    }
}
