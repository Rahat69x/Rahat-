/**
 * include/bank_api.h
 * Transport-agnostic service interface (Section 4.3).
 * Decouples the UI and client from whether the backend is local in-memory or remote over socket.
 */

#ifndef BANK_API_H
#define BANK_API_H

#include <stdint.h>
#include <stdbool.h>
#include "types.h"

typedef struct {
    bool success;
    int error_code;
    char message[128];
    char receipt_no[RECEIPT_ID_LEN + 1];
    int64_t balance_after;
} ApiResult;

typedef struct {
    void *ctx; /* Context pointer: Bank* for local, ClientSocket* for remote */
    
    ApiResult (*login)(void *ctx, long account_id, const char *pin, char out_name[MAX_NAME_LEN + 1]);
    ApiResult (*logout)(void *ctx, long account_id);
    ApiResult (*get_balance)(void *ctx, long account_id, int64_t *out_balance);
    ApiResult (*deposit)(void *ctx, long account_id, int64_t amount);
    ApiResult (*withdraw)(void *ctx, long account_id, int64_t amount);
    ApiResult (*transfer)(void *ctx, long src_id, long dst_id, int64_t amount, const char *otp);
    ApiResult (*get_history)(void *ctx, long account_id, Txn *out_txns, size_t max_count, size_t *out_count);
    ApiResult (*change_pin)(void *ctx, long account_id, const char *old_pin, const char *new_pin);
} BankApi;

/* Local in-process bank API provider */
BankApi bank_api_create_local(Bank *bank);

#endif /* BANK_API_H */
