/**
 * include/analytics.h
 * 60-column ASCII spending analytics, monthly charts, and customer budgets (F10).
 */

#ifndef ANALYTICS_H
#define ANALYTICS_H

#include "types.h"
#include <stdbool.h>

void analytics_render_spending(const Bank *bank, long account_id);

bool budget_set_limit(Bank *bank, long account_id, TxnType type, int64_t monthly_limit);

void budget_evaluate(Bank *bank, long account_id);

#endif /* ANALYTICS_H */
