/**
 * include/standing.h
 * Customer standing order creation, lifecycle, and automated execution (F9).
 */

#ifndef STANDING_H
#define STANDING_H

#include "types.h"
#include <stdbool.h>

bool standing_create(Bank *bank, long account_id, long dest_id, int64_t amount,
                     Frequency freq, int start_day, int count, uint64_t *out_id,
                     char *err_msg, size_t err_len);

bool standing_cancel(Bank *bank, long account_id, uint64_t order_id);

void standing_execute_due(Bank *bank);

#endif /* STANDING_H */
