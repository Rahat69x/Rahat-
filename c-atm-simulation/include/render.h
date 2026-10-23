/**
 * include/render.h
 * Masked display, receipt rendering, and formatted terminal output (C5).
 */

#ifndef RENDER_H
#define RENDER_H

#include "types.h"

void render_mask_account(long account_id, char *out, size_t out_len);
void render_receipt(const char *receipt_no, int day, time_t t, const char *type_str,
                    int64_t amount, int64_t balance_after, const char *currency,
                    long account_id, long counterparty);

#endif /* RENDER_H */
