/**
 * include/scheduler.h
 * Daily automated scheduling engine for standing orders, loan EMIs, interest, and fraud decay (F9).
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"

void scheduler_tick(Bank *bank);
void scheduler_advance_days(Bank *bank, int days);
int64_t scheduler_get_avg_balance(const Bank *bank, long account_id);

#endif /* SCHEDULER_H */
