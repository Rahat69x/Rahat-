/**
 * src/security/fraud.c
 * Fraud detection rule engine and security alerts (F2).
 */

#include "fraud.h"
#include "account.h"
#include "clock.h"
#include "audit.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static time_t g_last_rule_fire[MAX_ACCOUNTS][8];

void fraud_init(Bank *bank) {
    (void)bank;
    memset(g_last_rule_fire, 0, sizeof(g_last_rule_fire));
}

static int get_account_index(const Bank *bank, long account_id) {
    for (size_t i = 0; i < bank->account_count; ++i) {
        if (bank->accounts[i].id == account_id) return (int)i;
    }
    return -1;
}

static bool can_fire_rule(int acc_idx, int rule_id, time_t now) {
    if (acc_idx < 0 || acc_idx >= MAX_ACCOUNTS || rule_id < 1 || rule_id > 6) return false;
    if (g_last_rule_fire[acc_idx][rule_id] != 0 && (now - g_last_rule_fire[acc_idx][rule_id]) < 300) {
        return false;
    }
    return true;
}

static void mark_rule_fired(int acc_idx, int rule_id, time_t now) {
    if (acc_idx >= 0 && acc_idx < MAX_ACCOUNTS && rule_id >= 1 && rule_id <= 6) {
        g_last_rule_fire[acc_idx][rule_id] = now;
    }
}

static void apply_risk_points(Bank *bank, Account *acc, int acc_idx, int rule_id, int points, const char *msg) {
    time_t now = clock_now();
    mark_rule_fired(acc_idx, rule_id, now);
    acc->risk_score += points;
    if (acc->risk_score > 100) acc->risk_score = 100;

    char detail[96];
    snprintf(detail, sizeof(detail), "Rule R%d: %s (+%d pts, score=%d)", rule_id, msg, points, acc->risk_score);

    if (acc->risk_score >= bank->config.fraud_freeze) {
        acc->status = STATUS_FROZEN;
        fraud_add_alert(bank, acc->id, rule_id, acc->risk_score, "Account FROZEN due to critical fraud risk");
        audit_record(bank, "data", "SECURITY", "ACCOUNT_FREEZE", "-", detail);
    } else if (acc->risk_score >= bank->config.fraud_warn) {
        fraud_add_alert(bank, acc->id, rule_id, acc->risk_score, detail);
        audit_record(bank, "data", "SECURITY", "FRAUD_WARN", "-", detail);
    }
}

int fraud_eval_operation(Bank *bank, Account *acc, TxnType type, int64_t amount, long payee_id) {
    if (!bank || !acc) return 0;
    int acc_idx = get_account_index(bank, acc->id);
    time_t now = clock_now();
    int points_added = 0;

    /* R1: Velocity (4 or more money operations within 5 min) */
    int recent_count = 0;
    for (int i = 0; i < acc->recent_op_count; ++i) {
        if (now - acc->recent_op_times[i] <= 300) recent_count++;
    }
    if (recent_count >= 3 && can_fire_rule(acc_idx, FRAUD_RULE_VELOCITY, now)) {
        apply_risk_points(bank, acc, acc_idx, FRAUD_RULE_VELOCITY, 25, "Velocity limit exceeded");
        points_added += 25;
    }

    /* Record this operation timestamp */
    if (acc->recent_op_count < 8) {
        acc->recent_op_times[acc->recent_op_count++] = now;
    } else {
        memmove(&acc->recent_op_times[0], &acc->recent_op_times[1], 7 * sizeof(time_t));
        acc->recent_op_times[7] = now;
    }

    /* R2: Large amount (single operation >= 50% of daily_withdraw_limit) */
    int64_t daily_lim_minor = bank->config.daily_withdraw_limit * 100;
    if (amount >= (daily_lim_minor / 2) && can_fire_rule(acc_idx, FRAUD_RULE_LARGE_AMT, now)) {
        apply_risk_points(bank, acc, acc_idx, FRAUD_RULE_LARGE_AMT, 20, "Single operation >= 50% daily limit");
        points_added += 20;
    }

    /* R3: New payee */
    int64_t threshold_minor = bank->config.otp_transfer_threshold * 100;
    if (type == TXN_TRANSFER_OUT && amount >= threshold_minor &&
        can_fire_rule(acc_idx, FRAUD_RULE_NEW_PAYEE, now)) {
        bool previously_paid = false;
        for (size_t i = 0; i < bank->txn_count; ++i) {
            if (bank->txns[i].account == acc->id && bank->txns[i].type == TXN_TRANSFER_OUT &&
                bank->txns[i].counterparty == payee_id) {
                previously_paid = true;
                break;
            }
        }
        if (!previously_paid) {
            apply_risk_points(bank, acc, acc_idx, FRAUD_RULE_NEW_PAYEE, 25, "First transfer to new payee >= threshold");
            points_added += 25;
        }
    }

    /* R4: Odd hours (between 01:00 and 05:00) */
    struct tm *lt = localtime(&now);
    if (lt && lt->tm_hour >= 1 && lt->tm_hour < 5 && can_fire_rule(acc_idx, FRAUD_RULE_ODD_HOURS, now)) {
        apply_risk_points(bank, acc, acc_idx, FRAUD_RULE_ODD_HOURS, 10, "Operation during odd hours (01:00-05:00)");
        points_added += 10;
    }

    /* R6: Drain (takes >= 95% of balance) */
    if (acc->balance > 0 && amount >= (acc->balance * 95) / 100 && can_fire_rule(acc_idx, FRAUD_RULE_DRAIN, now)) {
        apply_risk_points(bank, acc, acc_idx, FRAUD_RULE_DRAIN, 20, "Account balance drain >= 95%");
        points_added += 20;
    }

    return points_added;
}

void fraud_record_failure(Bank *bank, long account_id, const char *reason) {
    if (!bank) return;
    Account *acc = account_find(bank, account_id);
    if (!acc) return;
    int acc_idx = get_account_index(bank, account_id);
    time_t now = clock_now();

    if (can_fire_rule(acc_idx, FRAUD_RULE_FAILURES, now)) {
        apply_risk_points(bank, acc, acc_idx, FRAUD_RULE_FAILURES, 20, reason ? reason : "Failed authentication/OTP");
    }
}

void fraud_decay_daily(Bank *bank) {
    if (!bank) return;
    for (size_t i = 0; i < bank->account_count; ++i) {
        if (bank->accounts[i].risk_score > 0) {
            bank->accounts[i].risk_score -= 10;
            if (bank->accounts[i].risk_score < 0) {
                bank->accounts[i].risk_score = 0;
            }
        }
    }
}

bool fraud_is_stepup_required(const Bank *bank, const Account *acc) {
    if (!bank || !acc) return false;
    return acc->risk_score >= bank->config.fraud_stepup;
}

bool fraud_is_frozen(const Account *acc) {
    return acc && acc->status == STATUS_FROZEN;
}

void fraud_add_alert(Bank *bank, long account_id, int rule_id, int score, const char *msg) {
    if (!bank || bank->alert_count >= MAX_ALERTS) return;
    Alert *al = &bank->alerts[bank->alert_count++];
    memset(al, 0, sizeof(Alert));
    al->id = ++bank->next_alert_id;
    al->day = bank->sim_day;
    al->account_id = account_id;
    al->rule_id = rule_id;
    al->risk_score = score;
    al->status = ALERT_NEW;
    if (msg) snprintf(al->message, sizeof(al->message), "%s", msg);
}

bool fraud_ack_alert(Bank *bank, uint64_t alert_id) {
    if (!bank) return false;
    for (size_t i = 0; i < bank->alert_count; ++i) {
        if (bank->alerts[i].id == alert_id) {
            bank->alerts[i].status = ALERT_ACKED;
            audit_record(bank, "data", "ADMIN", "ALERT_ACK", "-", "Acknowledged security alert");
            return true;
        }
    }
    return false;
}
