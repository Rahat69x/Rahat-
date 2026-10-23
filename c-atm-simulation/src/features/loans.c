/**
 * src/features/loans.c
 * Customer loan products, eligibility, amortization, and approval lifecycle (F5).
 */

#include "loans.h"
#include "scheduler.h"
#include "account.h"
#include "txn.h"
#include "audit.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

int64_t loan_calculate_emi(int64_t principal, int annual_rate_pct, int tenure_months) {
    if (principal <= 0 || tenure_months <= 0) return 0;
    double r = ((double)annual_rate_pct) / 12.0 / 100.0;
    double p = (double)principal;
    double pow_factor = pow(1.0 + r, (double)tenure_months);
    double emi = (p * r * pow_factor) / (pow_factor - 1.0);
    return (int64_t)llround(emi);
}

bool loan_generate_schedule(int64_t principal, int annual_rate_pct, int tenure_months,
                            AmortizationRow *out_rows, int max_rows) {
    if (principal <= 0 || tenure_months <= 0 || !out_rows || max_rows < tenure_months) return false;
    double r = ((double)annual_rate_pct) / 12.0 / 100.0;
    int64_t base_emi = loan_calculate_emi(principal, annual_rate_pct, tenure_months);
    int64_t outstanding = principal;

    for (int m = 1; m <= tenure_months; ++m) {
        AmortizationRow *row = &out_rows[m - 1];
        row->month = m;
        int64_t interest_part = (int64_t)llround((double)outstanding * r);

        if (m == tenure_months) {
            row->principal_part = outstanding;
            row->interest_part = interest_part;
            row->emi = row->principal_part + row->interest_part;
            row->balance_after = 0;
            outstanding = 0;
        } else {
            row->interest_part = interest_part;
            row->principal_part = base_emi - interest_part;
            outstanding -= row->principal_part;
            row->emi = base_emi;
            row->balance_after = outstanding;
        }
    }
    return true;
}

bool loan_check_eligibility(const Bank *bank, const Account *acc, int64_t requested_principal,
                            char *err_msg, size_t err_len) {
    if (!bank || !acc) return false;

    int account_age = bank->sim_day - acc->created_day;
    if (account_age < bank->config.loan_min_account_age_days) {
        if (err_msg) snprintf(err_msg, err_len, "Account age (%d days) below required minimum (%d days)",
                              account_age, bank->config.loan_min_account_age_days);
        return false;
    }
    if (acc->status != STATUS_ACTIVE) {
        if (err_msg) snprintf(err_msg, err_len, "Account must be in ACTIVE status to apply for loans");
        return false;
    }

    int active_loans = 0;
    for (size_t i = 0; i < bank->loan_count; ++i) {
        if (bank->loans[i].account_id == acc->id) {
            if (bank->loans[i].status == LOAN_OVERDUE) {
                if (err_msg) snprintf(err_msg, err_len, "Customer has an overdue loan. Ineligible.");
                return false;
            }
            if (bank->loans[i].status == LOAN_ACTIVE || bank->loans[i].status == LOAN_PENDING) {
                active_loans++;
            }
        }
    }
    if (active_loans >= bank->config.loan_max_active) {
        if (err_msg) snprintf(err_msg, err_len, "Maximum active loans limit (%d) reached", bank->config.loan_max_active);
        return false;
    }

    int64_t cap_minor = bank->config.loan_cap * 100;
    int64_t avg_bal = scheduler_get_avg_balance(bank, acc->id);
    int64_t max_allowed = 5 * avg_bal;
    if (max_allowed > cap_minor) max_allowed = cap_minor;
    if (requested_principal > max_allowed) {
        if (err_msg) snprintf(err_msg, err_len, "Requested principal exceeds limit (%s%.2f)",
                              bank->config.currency, (double)max_allowed / 100.0);
        return false;
    }

    return true;
}

bool loan_apply(Bank *bank, long account_id, LoanType type, int64_t principal,
                int tenure_months, uint64_t *out_loan_id, char *err_msg, size_t err_len) {
    if (!bank || bank->loan_count >= MAX_LOANS) return false;
    Account *acc = account_find(bank, account_id);
    if (!acc) return false;

    if (!loan_check_eligibility(bank, acc, principal, err_msg, err_len)) {
        return false;
    }

    int rate = (type == LOAN_PERSONAL) ? 12 : 8;
    int max_tenure = (type == LOAN_PERSONAL) ? 24 : 6;
    if (tenure_months <= 0 || tenure_months > max_tenure) {
        if (err_msg) snprintf(err_msg, err_len, "Invalid tenure for this loan product (max %d months)", max_tenure);
        return false;
    }

    Loan *ln = &bank->loans[bank->loan_count++];
    memset(ln, 0, sizeof(Loan));
    ln->id = ++bank->next_loan_id;
    ln->account_id = account_id;
    ln->type = type;
    ln->principal = principal;
    ln->interest_rate_pct = rate;
    ln->tenure_months = tenure_months;
    ln->emi = loan_calculate_emi(principal, rate, tenure_months);
    ln->remaining_balance = principal;
    ln->status = LOAN_PENDING;

    if (out_loan_id) *out_loan_id = ln->id;

    audit_record(bank, "data", "CUSTOMER", "LOAN_APPLY", "-", "Applied for loan product");
    return true;
}

Loan *loan_find(Bank *bank, uint64_t loan_id) {
    if (!bank) return NULL;
    for (size_t i = 0; i < bank->loan_count; ++i) {
        if (bank->loans[i].id == loan_id) return &bank->loans[i];
    }
    return NULL;
}

bool loan_approve(Bank *bank, const char *admin_id, uint64_t loan_id,
                  bool *out_needs_second_approval, char *err_msg, size_t err_len) {
    if (!bank || !admin_id) return false;
    Loan *ln = loan_find(bank, loan_id);
    if (!ln || ln->status != LOAN_PENDING) {
        if (err_msg) snprintf(err_msg, err_len, "Loan not found or not in PENDING state");
        return false;
    }

    int64_t dual_limit_minor = bank->config.loan_dual_approval_limit * 100;
    if (ln->principal > dual_limit_minor) {
        if (ln->approved_by[0] == '\0') {
            snprintf(ln->approved_by, sizeof(ln->approved_by), "%s", admin_id);
            if (out_needs_second_approval) *out_needs_second_approval = true;
            audit_record(bank, "data", admin_id, "LOAN_APPROVE_1", "-", "First approval recorded (dual required)");
            return true;
        } else if (strcmp(ln->approved_by, admin_id) == 0) {
            if (err_msg) snprintf(err_msg, err_len, "Dual approval requires a different administrator");
            return false;
        } else {
            snprintf(ln->second_approved_by, sizeof(ln->second_approved_by), "%s", admin_id);
            if (out_needs_second_approval) *out_needs_second_approval = false;
        }
    } else {
        snprintf(ln->approved_by, sizeof(ln->approved_by), "%s", admin_id);
        if (out_needs_second_approval) *out_needs_second_approval = false;
    }

    /* Final approval & disbursement */
    ln->status = LOAN_ACTIVE;
    ln->start_day = bank->sim_day;
    ln->next_emi_day = bank->sim_day + 30;

    Account *acc = account_find(bank, ln->account_id);
    if (acc) {
        acc->balance += ln->principal;
        txn_record(bank, acc->id, TXN_LOAN_DISBURSE, ln->principal, acc->balance, 0, "Loan disbursement", NULL);
    }

    audit_record(bank, "data", admin_id, "LOAN_DISBURSE", "-", "Approved and disbursed loan funds");
    return true;
}

bool loan_reject(Bank *bank, const char *admin_id, uint64_t loan_id,
                 char *err_msg, size_t err_len) {
    (void)err_msg; (void)err_len;
    Loan *ln = loan_find(bank, loan_id);
    if (!ln || ln->status != LOAN_PENDING) return false;
    ln->status = LOAN_REJECTED;
    audit_record(bank, "data", admin_id ? admin_id : "ADMIN", "LOAN_REJECT", "-", "Loan rejected");
    return true;
}

bool loan_prepay(Bank *bank, long account_id, uint64_t loan_id, int64_t prepay_amount,
                 bool is_full, char *err_msg, size_t err_len) {
    if (!bank) return false;
    Loan *ln = loan_find(bank, loan_id);
    if (!ln || ln->account_id != account_id || ln->status != LOAN_ACTIVE) {
        if (err_msg) snprintf(err_msg, err_len, "Active loan not found");
        return false;
    }
    Account *acc = account_find(bank, account_id);
    if (!acc) return false;

    int64_t debit = is_full ? ln->remaining_balance : prepay_amount;
    if (acc->balance < debit) {
        if (err_msg) snprintf(err_msg, err_len, "Insufficient funds to prepay loan");
        return false;
    }

    acc->balance -= debit;
    txn_record(bank, acc->id, TXN_LOAN_EMI, debit, acc->balance, 0, "Loan prepayment", NULL);

    if (is_full || debit >= ln->remaining_balance) {
        ln->remaining_balance = 0;
        ln->status = LOAN_PAID_OFF;
        audit_record(bank, "data", "CUSTOMER", "LOAN_CLOSE", "-", "Loan fully prepaid and closed");
    } else {
        ln->remaining_balance -= debit;
        /* Recalculate remaining EMI with tenure unchanged */
        int months_left = ln->tenure_months - ((bank->sim_day - ln->start_day) / 30);
        if (months_left <= 0) months_left = 1;
        ln->emi = loan_calculate_emi(ln->remaining_balance, ln->interest_rate_pct, months_left);
        audit_record(bank, "data", "CUSTOMER", "LOAN_PREPAY", "-", "Loan partial prepayment");
    }
    return true;
}
