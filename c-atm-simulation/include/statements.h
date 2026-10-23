/**
 * include/statements.h
 * Customer account statement generation, reconciliation, and RFC 4180 CSV/TXT export (F10).
 */

#ifndef STATEMENTS_H
#define STATEMENTS_H

#include "types.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    long account_id;
    int start_day;
    int end_day;
    int64_t opening_balance;
    int64_t total_in;
    int64_t total_out;
    int64_t closing_balance;
    bool reconciled;
    size_t txn_count;
    Txn txns[MAX_TXNS_TOTAL];
} Statement;

bool statement_generate(const Bank *bank, long account_id, int start_day, int end_day, Statement *out_stmt);

bool statement_export_txt(const Bank *bank, const Statement *stmt, const char *out_dir, char *out_filepath, size_t path_len);

bool statement_export_csv(const Statement *stmt, const char *out_dir, char *out_filepath, size_t path_len);

void statement_rfc4180_escape(const char *input, char *output, size_t out_len);

bool statement_rfc4180_unescape(const char *input, char *output, size_t out_len);

#endif /* STATEMENTS_H */
