/**
 * src/core/txn.c
 * Transaction recording and receipt generation (Section 4.2).
 */

#include "txn.h"
#include "clock.h"
#include <stdio.h>
#include <string.h>

bool txn_record(Bank *bank, long account_id, TxnType type, int64_t amount,
                int64_t balance_after, long counterparty, const char *note,
                char out_receipt[RECEIPT_ID_LEN + 1]) {
    if (!bank || bank->txn_count >= MAX_TXNS_TOTAL) return false;

    Txn *t = &bank->txns[bank->txn_count++];
    t->id = ++bank->next_txn_id;
    t->account = account_id;
    t->day = sim_day();
    t->clock_time = clock_now();
    t->type = type;
    t->amount = amount;
    t->balance_after = balance_after;
    t->counterparty = counterparty;

    if (note) {
        snprintf(t->note, sizeof(t->note), "%s", note);
    } else {
        t->note[0] = '\0';
    }

    snprintf(t->receipt_no, sizeof(t->receipt_no), "R-%d-%06llu", t->day, (unsigned long long)t->id);
    if (out_receipt) {
        snprintf(out_receipt, RECEIPT_ID_LEN + 1, "%s", t->receipt_no);
    }
    return true;
}

size_t txn_get_history(const Bank *bank, long account_id, Txn *out_txns, size_t max_count) {
    if (!bank || !out_txns || max_count == 0) return 0;
    size_t count = 0;
    for (size_t i = bank->txn_count; i > 0 && count < max_count; --i) {
        const Txn *t = &bank->txns[i - 1];
        if (t->account == account_id) {
            out_txns[count++] = *t;
        }
    }
    return count;
}
