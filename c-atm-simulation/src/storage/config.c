/**
 * src/storage/config.c
 * Hand-crafted key-value config.ini parser (Section 3).
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void config_set_defaults(Config *cfg) {
    if (!cfg) return;
    memset(cfg, 0, sizeof(Config));
    snprintf(cfg->currency, sizeof(cfg->currency), "$");
    cfg->withdraw_multiple = 100;
    cfg->per_withdraw_limit = 20000;
    cfg->daily_withdraw_limit = 50000;
    cfg->max_pin_tries = 3;
    cfg->pin_hash_iterations = 10000;
    cfg->otp_ttl_s = 120;
    cfg->otp_max_tries = 3;
    cfg->otp_resend_max = 2;
    cfg->otp_resend_gap_s = 30;
    cfg->otp_transfer_threshold = 10000;
    cfg->fraud_warn = 40;
    cfg->fraud_stepup = 70;
    cfg->fraud_freeze = 90;
    cfg->admin_max_tries = 3;
    cfg->loan_cap = 100000;
    cfg->loan_dual_approval_limit = 50000;
    cfg->loan_max_active = 2;
    cfg->loan_min_account_age_days = 30;
    cfg->late_fee_pct = 2;
    cfg->savings_rate_pct = 4;
    cfg->voucher_bad_tries = 5;
    snprintf(cfg->server_bind, sizeof(cfg->server_bind), "127.0.0.1");
    cfg->server_port = 5050;
    cfg->server_max_clients = 8;
    cfg->session_idle_s = 120;
}

static void trim(char *s) {
    char *p = s;
    while (isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
}

static void apply_kv(Config *cfg, const char *key, const char *val) {
    if (strcmp(key, "currency") == 0) {
        snprintf(cfg->currency, sizeof(cfg->currency), "%s", val);
    } else if (strcmp(key, "withdraw_multiple") == 0) {
        cfg->withdraw_multiple = strtoll(val, NULL, 10);
    } else if (strcmp(key, "per_withdraw_limit") == 0) {
        cfg->per_withdraw_limit = strtoll(val, NULL, 10);
    } else if (strcmp(key, "daily_withdraw_limit") == 0) {
        cfg->daily_withdraw_limit = strtoll(val, NULL, 10);
    } else if (strcmp(key, "max_pin_tries") == 0) {
        cfg->max_pin_tries = (int)strtol(val, NULL, 10);
    } else if (strcmp(key, "pin_hash_iterations") == 0) {
        cfg->pin_hash_iterations = (int)strtol(val, NULL, 10);
    } else if (strcmp(key, "otp_ttl_s") == 0) {
        cfg->otp_ttl_s = (int)strtol(val, NULL, 10);
    } else if (strcmp(key, "otp_max_tries") == 0) {
        cfg->otp_max_tries = (int)strtol(val, NULL, 10);
    } else if (strcmp(key, "otp_resend_max") == 0) {
        cfg->otp_resend_max = (int)strtol(val, NULL, 10);
    } else if (strcmp(key, "otp_resend_gap_s") == 0) {
        cfg->otp_resend_gap_s = (int)strtol(val, NULL, 10);
    } else if (strcmp(key, "otp_transfer_threshold") == 0) {
        cfg->otp_transfer_threshold = strtoll(val, NULL, 10);
    } else if (strcmp(key, "fraud_warn") == 0) {
        cfg->fraud_warn = (int)strtol(val, NULL, 10);
    } else if (strcmp(key, "fraud_stepup") == 0) {
        cfg->fraud_stepup = (int)strtol(val, NULL, 10);
    } else if (strcmp(key, "fraud_freeze") == 0) {
        cfg->fraud_freeze = (int)strtol(val, NULL, 10);
    } else if (strcmp(key, "admin_max_tries") == 0) {
        cfg->admin_max_tries = (int)strtol(val, NULL, 10);
    } else if (strcmp(key, "loan_cap") == 0) {
        cfg->loan_cap = strtoll(val, NULL, 10);
    } else if (strcmp(key, "loan_dual_approval_limit") == 0) {
        cfg->loan_dual_approval_limit = strtoll(val, NULL, 10);
    } else if (strcmp(key, "loan_max_active") == 0) {
        cfg->loan_max_active = (int)strtol(val, NULL, 10);
    } else if (strcmp(key, "loan_min_account_age_days") == 0) {
        cfg->loan_min_account_age_days = (int)strtol(val, NULL, 10);
    } else if (strcmp(key, "late_fee_pct") == 0) {
        cfg->late_fee_pct = (int)strtol(val, NULL, 10);
    } else if (strcmp(key, "savings_rate_pct") == 0) {
        cfg->savings_rate_pct = (int)strtol(val, NULL, 10);
    } else if (strcmp(key, "voucher_bad_tries") == 0) {
        cfg->voucher_bad_tries = (int)strtol(val, NULL, 10);
    } else if (strcmp(key, "server_bind") == 0) {
        snprintf(cfg->server_bind, sizeof(cfg->server_bind), "%s", val);
    } else if (strcmp(key, "server_port") == 0) {
        cfg->server_port = (int)strtol(val, NULL, 10);
    } else if (strcmp(key, "server_max_clients") == 0) {
        cfg->server_max_clients = (int)strtol(val, NULL, 10);
    } else if (strcmp(key, "session_idle_s") == 0) {
        cfg->session_idle_s = (int)strtol(val, NULL, 10);
    }
}

bool config_load(const char *filepath, Config *cfg) {
    config_set_defaults(cfg);
    FILE *fp = fopen(filepath, "r");
    if (!fp) return false;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char *p = line;
        while (isspace((unsigned char)*p)) p++;
        if (*p == '#' || *p == ';' || *p == '\0') continue;

        char *eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = p;
        char *val = eq + 1;
        trim(key);
        trim(val);
        apply_kv(cfg, key, val);
    }
    fclose(fp);
    return true;
}

bool config_save(const char *filepath, const Config *cfg) {
    if (!cfg) return false;
    FILE *fp = fopen(filepath, "w");
    if (!fp) return false;

    fprintf(fp, "# c-atm-simulation configuration\n");
    fprintf(fp, "currency = %s\n", cfg->currency);
    fprintf(fp, "withdraw_multiple = %lld\n", (long long)cfg->withdraw_multiple);
    fprintf(fp, "per_withdraw_limit = %lld\n", (long long)cfg->per_withdraw_limit);
    fprintf(fp, "daily_withdraw_limit = %lld\n", (long long)cfg->daily_withdraw_limit);
    fprintf(fp, "max_pin_tries = %d\n", cfg->max_pin_tries);
    fprintf(fp, "pin_hash_iterations = %d\n", cfg->pin_hash_iterations);
    fprintf(fp, "otp_ttl_s = %d\n", cfg->otp_ttl_s);
    fprintf(fp, "otp_max_tries = %d\n", cfg->otp_max_tries);
    fprintf(fp, "otp_resend_max = %d\n", cfg->otp_resend_max);
    fprintf(fp, "otp_resend_gap_s = %d\n", cfg->otp_resend_gap_s);
    fprintf(fp, "otp_transfer_threshold = %lld\n", (long long)cfg->otp_transfer_threshold);
    fprintf(fp, "fraud_warn = %d\n", cfg->fraud_warn);
    fprintf(fp, "fraud_stepup = %d\n", cfg->fraud_stepup);
    fprintf(fp, "fraud_freeze = %d\n", cfg->fraud_freeze);
    fprintf(fp, "admin_max_tries = %d\n", cfg->admin_max_tries);
    fprintf(fp, "loan_cap = %lld\n", (long long)cfg->loan_cap);
    fprintf(fp, "loan_dual_approval_limit = %lld\n", (long long)cfg->loan_dual_approval_limit);
    fprintf(fp, "loan_max_active = %d\n", cfg->loan_max_active);
    fprintf(fp, "loan_min_account_age_days = %d\n", cfg->loan_min_account_age_days);
    fprintf(fp, "late_fee_pct = %d\n", cfg->late_fee_pct);
    fprintf(fp, "savings_rate_pct = %d\n", cfg->savings_rate_pct);
    fprintf(fp, "voucher_bad_tries = %d\n", cfg->voucher_bad_tries);
    fprintf(fp, "server_bind = %s\n", cfg->server_bind);
    fprintf(fp, "server_port = %d\n", cfg->server_port);
    fprintf(fp, "server_max_clients = %d\n", cfg->server_max_clients);
    fprintf(fp, "session_idle_s = %d\n", cfg->session_idle_s);

    fclose(fp);
    return true;
}
