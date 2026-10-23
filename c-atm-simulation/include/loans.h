/**
 * include/loans.h
 * Customer loan products, eligibility, amortization, and approval lifecycle (F5).
 */

#ifndef LOANS_H
#define LOANS_H

#include "types.h"
#include <stdbool.h>

typedef struct {
    int month;
    int64_t emi;
    int64_t principal_part;
    int64_t interest_part;
    int64_t balance_after;
} AmortizationRow;

int64_t loan_calculate_emi(int64_t principal, int annual_rate_pct, int tenure_months);

bool loan_generate_schedule(int64_t principal, int annual_rate_pct, int tenure_months,
                            AmortizationRow *out_rows, int max_rows);

bool loan_check_eligibility(const Bank *bank, const Account *acc, int64_t requested_principal,
                            char *err_msg, size_t err_len);

bool loan_apply(Bank *bank, long account_id, LoanType type, int64_t principal,
                int tenure_months, uint64_t *out_loan_id, char *err_msg, size_t err_len);

bool loan_approve(Bank *bank, const char *admin_id, uint64_t loan_id,
                  bool *out_needs_second_approval, char *err_msg, size_t err_len);

bool loan_reject(Bank *bank, const char *admin_id, uint64_t loan_id,
                 char *err_msg, size_t err_len);

bool loan_prepay(Bank *bank, long account_id, uint64_t loan_id, int64_t prepay_amount,
                 bool is_full, char *err_msg, size_t err_len);

Loan *loan_find(Bank *bank, uint64_t loan_id);

#endif /* LOANS_H */
