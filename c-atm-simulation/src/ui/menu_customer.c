/**
 * src/ui/menu_customer.c
 * Interactive terminal menu adhering strictly to Section 4.4 numbering.
 */

#include "menu_customer.h"
#include "input.h"
#include "money.h"
#include "render.h"
#include "clock.h"
#include "account.h"
#include "loans.h"
#include "vouchers.h"
#include "fraud.h"
#include "otp.h"
#include "statements.h"
#include "analytics.h"
#include "standing.h"
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

static void handle_loans(BankApi *api, long account_id, const Config *cfg) {
    Bank *bank = (Bank *)api->ctx;
    printf("\n--- LOAN MANAGEMENT (F5) ---\n");
    printf("1. View Products & Calculator\n");
    printf("2. Apply for Loan\n");
    printf("3. My Active Loans & Prepay\n");
    printf("Choice: ");
    int sub = 0;
    if (!input_get_int(&sub, 1, 3)) return;

    if (sub == 1) {
        printf("\nAvailable Products:\n");
        printf("1. PERSONAL Loan: 12%% annual, up to 24 months\n");
        printf("2. SHORT Loan:    8%% annual, up to 6 months\n");
        printf("Enter Principal (%s): ", cfg->currency);
        long p_whole = 0;
        if (!input_get_long(&p_whole, 100, 1000000)) return;
        int64_t principal = (int64_t)p_whole * 100;
        printf("Enter Tenure (months): ");
        int tenure = 0;
        if (!input_get_int(&tenure, 1, 24)) return;

        int64_t emi = loan_calculate_emi(principal, 12, tenure);
        char emi_buf[32];
        money_format(emi, cfg->currency, emi_buf, sizeof(emi_buf));
        printf("\nEstimated Monthly EMI: %s\n", emi_buf);

        AmortizationRow rows[24];
        if (loan_generate_schedule(principal, 12, tenure, rows, 24)) {
            printf("\nAmortization Schedule (Sample):\n");
            printf("Mo | Monthly EMI | Principal | Interest | Balance\n");
            printf("---+-------------+-----------+----------+----------\n");
            for (int i = 0; i < tenure; ++i) {
                printf("%2d | %11.2f | %9.2f | %8.2f | %10.2f\n",
                       rows[i].month, (double)rows[i].emi / 100.0,
                       (double)rows[i].principal_part / 100.0,
                       (double)rows[i].interest_part / 100.0,
                       (double)rows[i].balance_after / 100.0);
            }
        }
    } else if (sub == 2) {
        printf("Product (1: Personal 12%%, 2: Short 8%%): ");
        int p_type = 1;
        if (!input_get_int(&p_type, 1, 2)) return;
        LoanType lt = (p_type == 1) ? LOAN_PERSONAL : LOAN_SHORT;
        printf("Requested Principal (%s): ", cfg->currency);
        long p_whole = 0;
        if (!input_get_long(&p_whole, 100, 1000000)) return;
        int64_t principal = (int64_t)p_whole * 100;
        printf("Tenure (months): ");
        int tenure = 0;
        if (!input_get_int(&tenure, 1, (lt == LOAN_PERSONAL ? 24 : 6))) return;

        /* Issue simulated OTP for loan application */
        otp_issue(bank, account_id, OTP_PURPOSE_LOAN, principal, NULL);
        printf("Enter 6-digit OTP code to submit application: ");
        char code[16];
        if (!input_get_line(code, sizeof(code))) return;

        char err[128];
        if (!otp_verify(bank, account_id, OTP_PURPOSE_LOAN, principal, code, err, sizeof(err))) {
            printf("OTP verification failed: %s\n", err);
            return;
        }

        uint64_t loan_id = 0;
        if (loan_apply(bank, account_id, lt, principal, tenure, &loan_id, err, sizeof(err))) {
            printf("Loan application submitted successfully! Application ID: #%llu (Status: PENDING approval)\n",
                   (unsigned long long)loan_id);
        } else {
            printf("Application rejected: %s\n", err);
        }
    } else if (sub == 3) {
        printf("\n--- Active Loans ---\n");
        int count = 0;
        for (size_t i = 0; i < bank->loan_count; ++i) {
            Loan *ln = &bank->loans[i];
            if (ln->account_id == account_id) {
                count++;
                printf("Loan #%llu: Principal: %s%.2f | Balance: %s%.2f | EMI: %s%.2f | Status: %s\n",
                       (unsigned long long)ln->id, cfg->currency, (double)ln->principal / 100.0,
                       cfg->currency, (double)ln->remaining_balance / 100.0,
                       cfg->currency, (double)ln->emi / 100.0,
                       ln->status == LOAN_ACTIVE ? "ACTIVE" : (ln->status == LOAN_PENDING ? "PENDING" : "OTHER"));
            }
        }
        if (count == 0) {
            printf("No loans found for this account.\n");
            return;
        }
        printf("\nPrepay a loan? (1: Full, 2: Partial, 0: No): ");
        int prep = 0;
        if (!input_get_int(&prep, 0, 2) || prep == 0) return;
        printf("Enter Loan ID to prepay: ");
        long lid = 0;
        if (!input_get_long(&lid, 1, 999999)) return;
        int64_t p_amt = 0;
        if (prep == 2) {
            printf("Enter prepayment amount (%s): ", cfg->currency);
            long whole = 0;
            if (!input_get_long(&whole, 1, 1000000)) return;
            p_amt = (int64_t)whole * 100;
        }
        char err[128];
        if (loan_prepay(bank, account_id, (uint64_t)lid, p_amt, prep == 1, err, sizeof(err))) {
            printf("Prepayment processed successfully!\n");
        } else {
            printf("Prepayment failed: %s\n", err);
        }
    }
}

static void handle_voucher(BankApi *api, long account_id) {
    Bank *bank = (Bank *)api->ctx;
    printf("\n--- REDEEM VOUCHER (F6) ---\n");
    printf("Enter 14-character Voucher Code (format: XXXX-XXXX-XXXX): ");
    char code[32];
    if (!input_get_line(code, sizeof(code))) return;

    char msg[128];
    if (voucher_redeem(bank, account_id, code, msg, sizeof(msg))) {
        printf("[SUCCESS] %s\n", msg);
    } else {
        printf("[ERROR] %s\n", msg);
    }
}

static void handle_alerts(BankApi *api, long account_id) {
    Bank *bank = (Bank *)api->ctx;
    Account *acc = account_find(bank, account_id);
    if (!acc) return;

    printf("\n--- SECURITY & FRAUD ALERTS (F2) ---\n");
    printf("Current Security Risk Score: %d / 100\n", acc->risk_score);
    if (acc->risk_score >= bank->config.fraud_freeze) {
        printf("STATUS: [CRITICAL] Account is FROZEN. Money operations disabled.\n");
    } else if (acc->risk_score >= bank->config.fraud_stepup) {
        printf("STATUS: [WARNING] Step-up verification (OTP) required for operations.\n");
    } else if (acc->risk_score >= bank->config.fraud_warn) {
        printf("STATUS: [NOTICE] Elevated security risk detected.\n");
    } else {
        printf("STATUS: [NORMAL] Account standing is good.\n");
    }

    printf("\nAlert History:\n");
    int count = 0;
    for (size_t i = 0; i < bank->alert_count; ++i) {
        if (bank->alerts[i].account_id == account_id) {
            count++;
            printf("Alert #%llu (Day %d): [Rule R%d] Score: %d | Status: %s | %s\n",
                   (unsigned long long)bank->alerts[i].id, bank->alerts[i].day,
                   bank->alerts[i].rule_id, bank->alerts[i].risk_score,
                   bank->alerts[i].status == ALERT_NEW ? "NEW" : "ACKNOWLEDGED",
                   bank->alerts[i].message);
        }
    }
    if (count == 0) {
        printf("No security incidents recorded.\n");
    }
}

static void handle_statements(BankApi *api, long account_id, const Config *cfg) {
    Bank *bank = (Bank *)api->ctx;
    printf("\n--- ACCOUNT STATEMENT & EXPORT (F10) ---\n");
    printf("Enter Start Day (1 - %d): ", bank->sim_day > 0 ? bank->sim_day : 1);
    int s_day = 1;
    if (!input_get_int(&s_day, 1, 10000)) return;
    printf("Enter End Day (%d - %d): ", s_day, bank->sim_day > 0 ? bank->sim_day : 1);
    int e_day = bank->sim_day > 0 ? bank->sim_day : s_day;
    if (!input_get_int(&e_day, s_day, 10000)) return;

    Statement stmt;
    if (!statement_generate(bank, account_id, s_day, e_day, &stmt)) {
        printf("Failed to generate statement.\n");
        return;
    }

    printf("\nStatement for Account ****%04ld (Period: Day %d - %d)\n", account_id % 10000, s_day, e_day);
    printf("Opening Balance: %s%.2f | Closing Balance: %s%.2f\n",
           cfg->currency, (double)stmt.opening_balance / 100.0,
           cfg->currency, (double)stmt.closing_balance / 100.0);
    printf("Total In:        %s%.2f | Total Out:       %s%.2f\n",
           cfg->currency, (double)stmt.total_in / 100.0,
           cfg->currency, (double)stmt.total_out / 100.0);
    printf("Reconciliation:  %s\n", stmt.reconciled ? "VERIFIED (MATCH)" : "STATEMENT MISMATCH");

    printf("\nExport Statement? (1: TXT, 2: RFC 4180 CSV, 0: No): ");
    int exp_opt = 0;
    if (input_get_int(&exp_opt, 0, 2) && exp_opt > 0) {
        char path[128];
        if (exp_opt == 1) {
            statement_export_txt(bank, &stmt, "statements", path, sizeof(path));
            printf("[EXPORT SUCCESS] Saved statement to '%s'\n", path);
        } else {
            statement_export_csv(&stmt, "statements", path, sizeof(path));
            printf("[EXPORT SUCCESS] Saved RFC 4180 CSV to '%s'\n", path);
        }
    }
}

static void handle_analytics(BankApi *api, long account_id, const Config *cfg) {
    Bank *bank = (Bank *)api->ctx;
    analytics_render_spending(bank, account_id);

    printf("\nBudget Options:\n");
    printf("1. Set Monthly Withdrawal Budget Limit\n");
    printf("2. Check Current Budget Alerts\n");
    printf("0. Back\nChoice: ");
    int b_opt = 0;
    if (input_get_int(&b_opt, 0, 2) && b_opt == 1) {
        printf("Enter Monthly Withdrawal Budget Limit (%s): ", cfg->currency);
        long b_limit = 0;
        if (input_get_long(&b_limit, 10, 1000000)) {
            budget_set_limit(bank, account_id, TXN_WITHDRAW, (int64_t)b_limit * 100);
            printf("Budget of %s%ld set successfully.\n", cfg->currency, b_limit);
            budget_evaluate(bank, account_id);
        }
    } else if (b_opt == 2) {
        budget_evaluate(bank, account_id);
        printf("Budget evaluation refreshed. Check Menu 12 for alerts.\n");
    }
}

static void handle_standing_orders(BankApi *api, long account_id, const Config *cfg) {
    Bank *bank = (Bank *)api->ctx;
    printf("\n--- STANDING ORDERS (F9) ---\n");
    printf("1. View Active Standing Orders\n");
    printf("2. Create New Standing Order (Requires OTP)\n");
    printf("3. Cancel Standing Order\n");
    printf("Choice: ");
    int so_choice = 0;
    if (!input_get_int(&so_choice, 1, 3)) return;

    if (so_choice == 1) {
        int count = 0;
        for (size_t i = 0; i < bank->standing_order_count; ++i) {
            StandingOrder *so = &bank->standing_orders[i];
            if (so->account_id == account_id && so->is_active) {
                count++;
                printf("SO #%llu -> Recipient: ****%04ld | Amount: %s%.2f | Freq: Every %d days | Next run: Day %d\n",
                       (unsigned long long)so->id, so->dest_account_id % 10000,
                       cfg->currency, (double)so->amount / 100.0,
                       (int)so->frequency, so->start_day);
            }
        }
        if (count == 0) printf("No active standing orders found.\n");
    } else if (so_choice == 2) {
        printf("Enter destination account number: ");
        long dst = 0;
        if (!input_get_long(&dst, 1000, 999999)) return;
        printf("Enter recurring transfer amount (%s): ", cfg->currency);
        long whole = 0;
        if (!input_get_long(&whole, 1, 1000000)) return;
        int64_t amt = (int64_t)whole * 100;
        printf("Frequency (1: Daily, 7: Weekly, 30: Monthly): ");
        int f_days = 1;
        if (!input_get_int(&f_days, 1, 30)) return;
        Frequency freq = (f_days >= 30) ? FREQ_MONTHLY : ((f_days >= 7) ? FREQ_WEEKLY : FREQ_DAILY);
        printf("Total execution count (e.g. 5, or 0 for unlimited): ");
        int exec_count = 0;
        input_get_int(&exec_count, 0, 100);

        /* OTP required for standing order creation (F1, F9) */
        otp_issue(bank, account_id, OTP_PURPOSE_TRANSFER, amt, NULL);
        printf("Enter 6-digit OTP code to authorize recurring order: ");
        char otp_code[16];
        if (!input_get_line(otp_code, sizeof(otp_code))) return;

        char err[128];
        if (!otp_verify(bank, account_id, OTP_PURPOSE_TRANSFER, amt, otp_code, err, sizeof(err))) {
            printf("OTP verification failed: %s\n", err);
            return;
        }

        uint64_t so_id = 0;
        if (standing_create(bank, account_id, dst, amt, freq, bank->sim_day + 1, exec_count, &so_id, err, sizeof(err))) {
            printf("[SUCCESS] Standing order #%llu created successfully!\n", (unsigned long long)so_id);
        } else {
            printf("[ERROR] Failed to create standing order: %s\n", err);
        }
    } else if (so_choice == 3) {
        printf("Enter Standing Order ID to cancel: ");
        long so_id = 0;
        if (!input_get_long(&so_id, 1, 999999)) return;
        if (standing_cancel(bank, account_id, (uint64_t)so_id)) {
            printf("[SUCCESS] Standing order cancelled.\n");
        } else {
            printf("[ERROR] Standing order not found or already cancelled.\n");
        }
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
            case 7: handle_statements(api, id, cfg); break;
            case 8: handle_analytics(api, id, cfg); break;
            case 9: handle_standing_orders(api, id, cfg); break;
            case 10: handle_loans(api, id, cfg); break;
            case 11: handle_voucher(api, id); break;
            case 12: handle_alerts(api, id); break;
            case 0:
                api->logout(api->ctx, id);
                in_session = false;
                printf("Logged out cleanly.\n");
                break;
            default: break;
        }
    }
}
