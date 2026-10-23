/**
 * src/features/scheduler.c
 * Daily automated scheduling engine with 9-step tick order and idempotent job ledger (F9).
 */

#include "scheduler.h"
#include "account.h"
#include "standing.h"
#include "loans.h"
#include "fraud.h"
#include "txn.h"
#include "audit.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static void step1_snapshot(Bank *bank) {
    if (bank->ledger.last_snapshot_day == bank->sim_day) return;
    for (size_t i = 0; i < bank->account_count; ++i) {
        int idx = bank->sim_day % 30;
        bank->balance_snapshots[i][idx] = bank->accounts[i].balance;
        if (bank->snapshot_count[i] < 30) {
            bank->snapshot_count[i]++;
        }
    }
    bank->ledger.last_snapshot_day = bank->sim_day;
}

static void step2_standing_orders(Bank *bank) {
    if (bank->ledger.last_standing_day == bank->sim_day) return;
    standing_execute_due(bank);
    bank->ledger.last_standing_day = bank->sim_day;
}

static void step3_and_4_loans(Bank *bank) {
    if (bank->ledger.last_loan_day == bank->sim_day) return;
    for (size_t i = 0; i < bank->loan_count; ++i) {
        Loan *ln = &bank->loans[i];
        if (ln->status != LOAN_ACTIVE || bank->sim_day < ln->next_emi_day) continue;

        Account *acc = account_find(bank, ln->account_id);
        if (!acc) continue;

        if (acc->balance >= ln->emi) {
            acc->balance -= ln->emi;
            double r = ((double)ln->interest_rate_pct) / 12.0 / 100.0;
            int64_t interest_part = (int64_t)llround((double)ln->remaining_balance * r);
            int64_t principal_part = ln->emi - interest_part;
            if (principal_part >= ln->remaining_balance || ln->remaining_balance <= ln->emi) {
                ln->remaining_balance = 0;
                ln->status = LOAN_PAID_OFF;
            } else {
                ln->remaining_balance -= principal_part;
                ln->next_emi_day += 30;
            }
            txn_record(bank, acc->id, TXN_LOAN_EMI, ln->emi, acc->balance, 0, "Loan EMI auto-debit", NULL);
            ln->missed_emis = 0;
        } else {
            ln->missed_emis++;
            int64_t fee = (ln->emi * bank->config.late_fee_pct) / 100;
            acc->balance -= fee;
            txn_record(bank, acc->id, TXN_FEE, fee, acc->balance, 0, "Loan late fee penalty", NULL);
            if (ln->missed_emis >= 2) {
                ln->status = LOAN_OVERDUE;
                fraud_add_alert(bank, acc->id, 0, acc->risk_score, "Loan OVERDUE: 2 missed EMIs");
            }
            ln->next_emi_day += 30;
        }
    }
    bank->ledger.last_loan_day = bank->sim_day;
}

static void step5_interest(Bank *bank) {
    if (bank->ledger.last_interest_day == bank->sim_day) return;
    for (size_t i = 0; i < bank->account_count; ++i) {
        Account *a = &bank->accounts[i];
        if (a->status != STATUS_ACTIVE || a->balance <= 0) continue;

        int64_t daily_gain = a->balance * (int64_t)bank->config.savings_rate_pct;
        a->interest_remainder += daily_gain;

        if (bank->sim_day % 30 == 0) {
            int64_t credit = a->interest_remainder / 36500LL;
            a->interest_remainder %= 36500LL;
            if (credit > 0) {
                a->balance += credit;
                txn_record(bank, a->id, TXN_INTEREST, credit, a->balance, 0, "Monthly interest credit", NULL);
            }
        }
    }
    bank->ledger.last_interest_day = bank->sim_day;
}

static void step6_vouchers(Bank *bank) {
    if (bank->ledger.last_voucher_day == bank->sim_day) return;
    for (size_t i = 0; i < bank->voucher_count; ++i) {
        Voucher *v = &bank->vouchers[i];
        if (v->status == VOUCHER_ACTIVE && v->expiry_day > 0 && bank->sim_day >= v->expiry_day) {
            v->status = VOUCHER_EXPIRED;
        }
    }
    bank->ledger.last_voucher_day = bank->sim_day;
}

static void step7_reset_limits(Bank *bank) {
    if (bank->ledger.last_reset_day == bank->sim_day) return;
    for (size_t i = 0; i < bank->account_count; ++i) {
        bank->accounts[i].withdrawn_today = 0;
        bank->accounts[i].last_withdraw_day = bank->sim_day;
    }
    bank->ledger.last_reset_day = bank->sim_day;
}

static void step8_fraud_decay(Bank *bank) {
    if (bank->ledger.last_fraud_day == bank->sim_day) return;
    fraud_decay_daily(bank);
    bank->ledger.last_fraud_day = bank->sim_day;
}

static void step9_day_end(Bank *bank) {
    if (bank->ledger.last_day_end == bank->sim_day) return;
    char detail[64];
    snprintf(detail, sizeof(detail), "Sim day %d completed", bank->sim_day);
    audit_record(bank, "data", "SYSTEM", "DAY_END", "-", detail);
    bank->ledger.last_day_end = bank->sim_day;
}

void scheduler_tick(Bank *bank) {
    if (!bank) return;
    step1_snapshot(bank);
    step2_standing_orders(bank);
    step3_and_4_loans(bank);
    step5_interest(bank);
    step6_vouchers(bank);
    step7_reset_limits(bank);
    step8_fraud_decay(bank);
    step9_day_end(bank);
}

void scheduler_advance_days(Bank *bank, int days) {
    if (!bank || days <= 0) return;
    for (int d = 0; d < days; ++d) {
        bank->sim_day++;
        scheduler_tick(bank);
    }
}

int64_t scheduler_get_avg_balance(const Bank *bank, long account_id) {
    if (!bank) return 0;
    for (size_t i = 0; i < bank->account_count; ++i) {
        if (bank->accounts[i].id == account_id) {
            int count = bank->snapshot_count[i];
            if (count == 0) return bank->accounts[i].balance;
            int64_t sum = 0;
            for (int k = 0; k < count; ++k) {
                sum += bank->balance_snapshots[i][k];
            }
            return sum / count;
        }
    }
    return 0;
}
