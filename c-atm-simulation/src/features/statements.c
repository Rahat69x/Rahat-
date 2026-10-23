/**
 * src/features/statements.c
 * Customer account statement generation, reconciliation, and RFC 4180 CSV/TXT export (F10).
 */

#include "statements.h"
#include "account.h"
#include "audit.h"
#include <stdio.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>
#define MAKE_DIR(d) _mkdir(d)
#else
#include <sys/stat.h>
#define MAKE_DIR(d) mkdir(d, 0755)
#endif

void statement_rfc4180_escape(const char *in, char *out, size_t out_len) {
    if (!in || !out || out_len == 0) return;
    bool needs_quotes = (strchr(in, ',') || strchr(in, '"') || strchr(in, '\n') || strchr(in, '\r'));
    size_t pos = 0;
    if (needs_quotes && pos < out_len - 1) out[pos++] = '"';

    for (size_t i = 0; in[i] != '\0' && pos < out_len - 1; ++i) {
        if (in[i] == '"') {
            if (pos < out_len - 2) {
                out[pos++] = '"';
                out[pos++] = '"';
            }
        } else {
            out[pos++] = in[i];
        }
    }
    if (needs_quotes && pos < out_len - 1) out[pos++] = '"';
    out[pos] = '\0';
}

bool statement_rfc4180_unescape(const char *in, char *out, size_t out_len) {
    if (!in || !out || out_len == 0) return false;
    size_t len = strlen(in);
    if (len >= 2 && in[0] == '"' && in[len - 1] == '"') {
        size_t pos = 0;
        for (size_t i = 1; i < len - 1 && pos < out_len - 1; ++i) {
            if (in[i] == '"' && in[i + 1] == '"') {
                out[pos++] = '"';
                i++;
            } else {
                out[pos++] = in[i];
            }
        }
        out[pos] = '\0';
    } else {
        snprintf(out, out_len, "%s", in);
    }
    return true;
}

static bool is_credit_type(TxnType t) {
    return (t == TXN_DEPOSIT || t == TXN_TRANSFER_IN || t == TXN_LOAN_DISBURSE ||
            t == TXN_INTEREST || t == TXN_VOUCHER);
}

bool statement_generate(const Bank *bank, long account_id, int start_day, int end_day, Statement *stmt) {
    if (!bank || !stmt) return false;
    memset(stmt, 0, sizeof(Statement));
    stmt->account_id = account_id;
    stmt->start_day = start_day;
    stmt->end_day = end_day;

    int64_t running_bal = 0;
    bool seen_prev = false;

    for (size_t i = 0; i < bank->txn_count; ++i) {
        const Txn *t = &bank->txns[i];
        if (t->account != account_id) continue;

        if (t->day < start_day) {
            running_bal = t->balance_after;
            seen_prev = true;
        } else if (t->day <= end_day) {
            if (!seen_prev && stmt->txn_count == 0) {
                running_bal = is_credit_type(t->type) ? (t->balance_after - t->amount) : (t->balance_after + t->amount);
                seen_prev = true;
            }
            if (stmt->txn_count < MAX_TXNS_TOTAL) {
                stmt->txns[stmt->txn_count++] = *t;
            }
            if (is_credit_type(t->type)) {
                stmt->total_in += t->amount;
            } else {
                stmt->total_out += t->amount;
            }
            stmt->closing_balance = t->balance_after;
        }
    }

    stmt->opening_balance = running_bal;
    if (stmt->txn_count == 0) {
        const Account *acc = account_find((Bank *)bank, account_id);
        stmt->opening_balance = acc ? acc->balance : 0;
        stmt->closing_balance = stmt->opening_balance;
    }

    stmt->reconciled = (stmt->opening_balance + stmt->total_in - stmt->total_out == stmt->closing_balance);
    if (!stmt->reconciled) {
        printf("STATEMENT MISMATCH for account ****%04ld: Open(%lld) + In(%lld) - Out(%lld) != Close(%lld)\n",
               account_id % 10000, (long long)stmt->opening_balance, (long long)stmt->total_in,
               (long long)stmt->total_out, (long long)stmt->closing_balance);
        audit_record((Bank *)bank, "data", "SYSTEM", "STATEMENT_MISMATCH", "-", "Statement reconciliation mismatch");
    }
    return true;
}

bool statement_export_txt(const Bank *bank, const Statement *stmt, const char *out_dir, char *out_path, size_t path_len) {
    if (!stmt) return false;
    const char *dir = out_dir ? out_dir : "statements";
    MAKE_DIR(dir);
    snprintf(out_path, path_len, "%s/%ld_%d_%d.txt", dir, stmt->account_id, stmt->start_day, stmt->end_day);

    FILE *f = fopen(out_path, "w");
    if (!f) return false;

    const char *sym = bank ? bank->config.currency : "$";
    fprintf(f, "============================================================\n");
    fprintf(f, "                   ACCOUNT STATEMENT                        \n");
    fprintf(f, "============================================================\n");
    fprintf(f, "Account: ****%04ld               Period: Day %d - Day %d\n",
            stmt->account_id % 10000, stmt->start_day, stmt->end_day);
    fprintf(f, "Opening Balance: %s%.2f           Closing Balance: %s%.2f\n",
            sym, (double)stmt->opening_balance / 100.0, sym, (double)stmt->closing_balance / 100.0);
    fprintf(f, "Total In:        %s%.2f           Total Out:       %s%.2f\n",
            sym, (double)stmt->total_in / 100.0, sym, (double)stmt->total_out / 100.0);
    fprintf(f, "Reconciliation:  %s\n", stmt->reconciled ? "VERIFIED (MATCH)" : "STATEMENT MISMATCH");
    fprintf(f, "------------------------------------------------------------\n");
    fprintf(f, "Day  Receipt       Type         Amount        Balance       Note\n");
    fprintf(f, "------------------------------------------------------------\n");

    for (size_t i = 0; i < stmt->txn_count; ++i) {
        const Txn *t = &stmt->txns[i];
        bool cred = is_credit_type(t->type);
        fprintf(f, "%-4d %-13s %-12s %c%s%.2f   %s%.2f   %s\n",
                t->day, t->receipt_no, (cred ? "CREDIT" : "DEBIT"),
                (cred ? '+' : '-'), sym, (double)t->amount / 100.0,
                sym, (double)t->balance_after / 100.0, t->note);
    }
    fprintf(f, "============================================================\n");
    fclose(f);
    return true;
}

bool statement_export_csv(const Statement *stmt, const char *out_dir, char *out_path, size_t path_len) {
    if (!stmt) return false;
    const char *dir = out_dir ? out_dir : "statements";
    MAKE_DIR(dir);
    snprintf(out_path, path_len, "%s/%ld_%d_%d.csv", dir, stmt->account_id, stmt->start_day, stmt->end_day);

    FILE *f = fopen(out_path, "w");
    if (!f) return false;

    fprintf(f, "Day,Receipt,Type,Amount,BalanceAfter,Counterparty,Note\n");
    for (size_t i = 0; i < stmt->txn_count; ++i) {
        const Txn *t = &stmt->txns[i];
        char esc_note[128];
        statement_rfc4180_escape(t->note, esc_note, sizeof(esc_note));
        fprintf(f, "%d,%s,%d,%lld,%lld,%ld,%s\n",
                t->day, t->receipt_no, (int)t->type, (long long)t->amount,
                (long long)t->balance_after, t->counterparty, esc_note);
    }
    fclose(f);
    return true;
}
