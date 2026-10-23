/**
 * src/features/analytics.c
 * 60-column ASCII spending analytics, monthly charts, and customer budgets (F10).
 */

#include "analytics.h"
#include "fraud.h"
#include <stdio.h>
#include <string.h>

static bool is_debit(TxnType t) {
    return (t == TXN_WITHDRAW || t == TXN_TRANSFER_OUT || t == TXN_LOAN_EMI ||
            t == TXN_FEE || t == TXN_STANDING_ORDER);
}

static void render_barchart(const Bank *bank, long account_id, const char *sym) {
    int64_t monthly_out[6] = {0};
    int current_month = bank->sim_day / 30;

    for (size_t i = 0; i < bank->txn_count; ++i) {
        const Txn *t = &bank->txns[i];
        if (t->account != account_id || !is_debit(t->type)) continue;
        int m = t->day / 30;
        int diff = current_month - m;
        if (diff >= 0 && diff < 6) {
            monthly_out[5 - diff] += t->amount;
        }
    }

    int64_t max_val = 0;
    for (int i = 0; i < 6; ++i) {
        if (monthly_out[i] > max_val) max_val = monthly_out[i];
    }

    printf("--- MONTHLY OUTFLOW (LAST 6 MONTHS) ------------------------\n");
    for (int i = 0; i < 6; ++i) {
        char bar[26];
        int bar_len = 0;
        if (max_val > 0) {
            bar_len = (int)((monthly_out[i] * 24) / max_val);
            if (bar_len > 24) bar_len = 24;
        }
        for (int k = 0; k < bar_len; ++k) bar[k] = '#';
        for (int k = bar_len; k < 24; ++k) bar[k] = ' ';
        bar[24] = '\0';
        printf(" M%-2d [%s] %s%8.2f\n", i + 1, bar, sym, (double)monthly_out[i] / 100.0);
    }
}

static void render_totals_and_top5(const Bank *bank, long account_id, const char *sym) {
    int64_t sum_withdraw = 0, sum_transfer = 0, sum_fee = 0, sum_other = 0, total_out = 0;
    Txn top[5];
    int top_count = 0;

    for (size_t i = 0; i < bank->txn_count; ++i) {
        const Txn *t = &bank->txns[i];
        if (t->account != account_id || !is_debit(t->type)) continue;
        total_out += t->amount;
        if (t->type == TXN_WITHDRAW) sum_withdraw += t->amount;
        else if (t->type == TXN_TRANSFER_OUT) sum_transfer += t->amount;
        else if (t->type == TXN_FEE) sum_fee += t->amount;
        else sum_other += t->amount;

        /* Maintain top 5 largest */
        if (top_count < 5) {
            top[top_count++] = *t;
        } else {
            int min_idx = 0;
            for (int k = 1; k < 5; ++k) {
                if (top[k].amount < top[min_idx].amount) min_idx = k;
            }
            if (t->amount > top[min_idx].amount) top[min_idx] = *t;
        }
    }

    int days = bank->sim_day > 0 ? bank->sim_day : 1;
    printf("--- SPENDING BREAKDOWN & METRICS ---------------------------\n");
    printf(" Withdrawals:  %s%-10.2f  Transfers Out: %s%.2f\n", sym, (double)sum_withdraw / 100.0, sym, (double)sum_transfer / 100.0);
    printf(" Fees/Penalties: %s%-8.2f  Other Debits:  %s%.2f\n", sym, (double)sum_fee / 100.0, sym, (double)sum_other / 100.0);
    printf(" Avg Daily Spend: %s%.2f (Total Out: %s%.2f)\n", sym, ((double)total_out / (double)days) / 100.0, sym, (double)total_out / 100.0);

    printf("--- TOP 5 LARGEST TRANSACTIONS -----------------------------\n");
    for (int k = 0; k < top_count; ++k) {
        printf(" #%d Day %-3d %-12s %s%.2f (%s)\n", k + 1, top[k].day, top[k].receipt_no, sym, (double)top[k].amount / 100.0, top[k].note);
    }
}

void analytics_render_spending(const Bank *bank, long account_id) {
    if (!bank) return;
    const char *sym = bank->config.currency;
    printf("============================================================\n");
    printf("              ACCOUNT SPENDING ANALYTICS (60 COL)           \n");
    printf("============================================================\n");
    printf("Account: ****%04ld                 Current Day: %d\n", account_id % 10000, bank->sim_day);
    render_barchart(bank, account_id, sym);
    render_totals_and_top5(bank, account_id, sym);
    printf("============================================================\n");
}

bool budget_set_limit(Bank *bank, long account_id, TxnType type, int64_t monthly_limit) {
    if (!bank) return false;
    for (size_t i = 0; i < bank->budget_count; ++i) {
        if (bank->budgets[i].account_id == account_id && bank->budgets[i].type == type) {
            bank->budgets[i].monthly_limit = monthly_limit;
            bank->budgets[i].is_active = (monthly_limit > 0);
            return true;
        }
    }
    if (bank->budget_count < MAX_ACCOUNTS) {
        AccountBudget *b = &bank->budgets[bank->budget_count++];
        b->account_id = account_id;
        b->type = type;
        b->monthly_limit = monthly_limit;
        b->is_active = (monthly_limit > 0);
        return true;
    }
    return false;
}

void budget_evaluate(Bank *bank, long account_id) {
    if (!bank) return;
    int cur_month_start = (bank->sim_day / 30) * 30;

    for (size_t i = 0; i < bank->budget_count; ++i) {
        AccountBudget *b = &bank->budgets[i];
        if (b->account_id != account_id || !b->is_active || b->monthly_limit <= 0) continue;

        int64_t spent = 0;
        for (size_t k = 0; k < bank->txn_count; ++k) {
            const Txn *t = &bank->txns[k];
            if (t->account == account_id && t->type == b->type && t->day >= cur_month_start) {
                spent += t->amount;
            }
        }

        if (spent >= b->monthly_limit) {
            fraud_add_alert(bank, account_id, 0, 0, "[BUDGET EXCEEDED] 100% of monthly budget limit spent");
        } else if (spent >= (b->monthly_limit * 8) / 10) {
            fraud_add_alert(bank, account_id, 0, 0, "[BUDGET NOTICE] 80% of monthly budget limit reached");
        }
    }
}
