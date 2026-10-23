/**
 * include/account.h
 * Customer account domain logic and queries (Section 4.2).
 */

#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "types.h"
#include <stdbool.h>

Account *account_find(Bank *bank, long id);
Account *account_create(Bank *bank, long id, const char *name, int64_t initial_balance, const char *pin);
bool account_can_withdraw(const Bank *bank, const Account *acc, int64_t amount, char *reason, size_t reason_len);
void account_reset_daily_limits_if_needed(Account *acc, int current_day);

#endif /* ACCOUNT_H */
