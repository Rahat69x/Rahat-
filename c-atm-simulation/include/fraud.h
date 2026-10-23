/**
 * include/fraud.h
 * Fraud detection rule engine and security alerts (F2).
 */

#ifndef FRAUD_H
#define FRAUD_H

#include "types.h"
#include <stdbool.h>

#define FRAUD_RULE_VELOCITY    1
#define FRAUD_RULE_LARGE_AMT   2
#define FRAUD_RULE_NEW_PAYEE   3
#define FRAUD_RULE_ODD_HOURS   4
#define FRAUD_RULE_FAILURES    5
#define FRAUD_RULE_DRAIN       6

void fraud_init(Bank *bank);

int fraud_eval_operation(Bank *bank, Account *acc, TxnType type, int64_t amount, long payee_id);

void fraud_record_failure(Bank *bank, long account_id, const char *reason);

void fraud_decay_daily(Bank *bank);

bool fraud_is_stepup_required(const Bank *bank, const Account *acc);

bool fraud_is_frozen(const Account *acc);

void fraud_add_alert(Bank *bank, long account_id, int rule_id, int score, const char *msg);

bool fraud_ack_alert(Bank *bank, uint64_t alert_id);

#endif /* FRAUD_H */
