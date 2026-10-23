/**
 * include/txn.h
 * Transaction recording and receipt generation (Section 4.2).
 */

#ifndef TXN_H
#define TXN_H

#include "types.h"
#include <stdbool.h>

bool txn_record(Bank *bank, long account_id, TxnType type, int64_t amount,
                int64_t balance_after, long counterparty, const char *note,
                char out_receipt[RECEIPT_ID_LEN + 1]);

size_t txn_get_history(const Bank *bank, long account_id, Txn *out_txns, size_t max_count);

#endif /* TXN_H */
