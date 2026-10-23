/**
 * src/features/vouchers.c
 * Promotional and credit voucher management (F6).
 */

#include "vouchers.h"
#include "sha256.h"
#include "rng.h"
#include "clock.h"
#include "audit.h"
#include "account.h"
#include "txn.h"
#include "fraud.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_VOUCHER_USAGES 256

typedef struct {
    uint64_t voucher_id;
    long account_id;
    int count;
} VoucherUsage;

static VoucherUsage g_usages[MAX_VOUCHER_USAGES];
static size_t g_usage_count = 0;

static int g_bad_tries[MAX_ACCOUNTS];
static time_t g_blocked_until[MAX_ACCOUNTS];

static int char_to_val(char c) {
    char up = (char)toupper((unsigned char)c);
    const char *p = strchr(VOUCHER_ALPHABET, up);
    return p ? (int)(p - VOUCHER_ALPHABET) : -1;
}

bool voucher_validate_code_format(const char *code_str, char *out_clean_12) {
    if (!code_str) return false;
    char clean[16];
    size_t len = 0;

    for (size_t i = 0; code_str[i] != '\0'; ++i) {
        if (code_str[i] == '-' || code_str[i] == ' ') continue;
        if (len >= 12) return false;
        int v = char_to_val(code_str[i]);
        if (v < 0) return false;
        clean[len++] = (char)toupper((unsigned char)code_str[i]);
    }
    if (len != 12) return false;
    clean[12] = '\0';

    int sum = 0;
    for (int i = 0; i < 11; ++i) {
        sum += char_to_val(clean[i]) * (i + 1);
    }
    int expected_check = sum % 32;
    int actual_check = char_to_val(clean[11]);
    if (actual_check != expected_check) {
        return false;
    }

    if (out_clean_12) {
        memcpy(out_clean_12, clean, 13);
    }
    return true;
}

bool voucher_generate_code(char *out_formatted_15) {
    if (!out_formatted_15) return false;
    char clean[13];
    int sum = 0;

    for (int i = 0; i < 11; ++i) {
        int idx = (int)(rng_u32() % 32);
        clean[i] = VOUCHER_ALPHABET[idx];
        sum += idx * (i + 1);
    }
    clean[11] = VOUCHER_ALPHABET[sum % 32];
    clean[12] = '\0';

    snprintf(out_formatted_15, 15, "%.4s-%.4s-%.4s", clean, clean + 4, clean + 8);
    return true;
}

bool voucher_create(Bank *bank, const char *admin_id, VoucherType type, int64_t value,
                    int expiry_day, int max_uses, int uses_per_acc, char *out_code_str) {
    if (!bank || bank->voucher_count >= MAX_VOUCHERS) return false;

    char formatted[16];
    if (!voucher_generate_code(formatted)) return false;

    char clean[13];
    voucher_validate_code_format(formatted, clean);

    Voucher *v = &bank->vouchers[bank->voucher_count++];
    memset(v, 0, sizeof(Voucher));
    v->id = ++bank->next_voucher_id;
    v->type = type;
    v->value = value;
    v->expiry_day = expiry_day;
    v->max_total_uses = max_uses;
    v->total_uses = 0;
    v->uses_per_account = uses_per_acc;
    v->status = VOUCHER_ACTIVE;
    if (admin_id) snprintf(v->created_by, sizeof(v->created_by), "%s", admin_id);

    sha256_hash((const uint8_t *)clean, 12, v->code_hash);

    if (out_code_str) {
        snprintf(out_code_str, 16, "%s", formatted);
    }

    audit_record(bank, "data", admin_id ? admin_id : "ADMIN", "VOUCHER_CREATE", "-", "Created promotional voucher");
    return true;
}

static int get_account_usage(uint64_t voucher_id, long account_id) {
    for (size_t i = 0; i < g_usage_count; ++i) {
        if (g_usages[i].voucher_id == voucher_id && g_usages[i].account_id == account_id) {
            return g_usages[i].count;
        }
    }
    return 0;
}

static void record_account_usage(uint64_t voucher_id, long account_id) {
    for (size_t i = 0; i < g_usage_count; ++i) {
        if (g_usages[i].voucher_id == voucher_id && g_usages[i].account_id == account_id) {
            g_usages[i].count++;
            return;
        }
    }
    if (g_usage_count < MAX_VOUCHER_USAGES) {
        g_usages[g_usage_count].voucher_id = voucher_id;
        g_usages[g_usage_count].account_id = account_id;
        g_usages[g_usage_count].count = 1;
        g_usage_count++;
    }
}

bool voucher_redeem(Bank *bank, long account_id, const char *code_str,
                    char *out_msg, size_t msg_len) {
    if (!bank) return false;
    Account *acc = account_find(bank, account_id);
    if (!acc) return false;

    int acc_idx = 0;
    for (size_t i = 0; i < bank->account_count; ++i) {
        if (bank->accounts[i].id == account_id) { acc_idx = (int)i; break; }
    }

    time_t now = clock_now();
    if (g_blocked_until[acc_idx] > now) {
        int remain = (int)((g_blocked_until[acc_idx] - now) / 60);
        if (out_msg) snprintf(out_msg, msg_len, "Voucher entry Locked. Please try again in %d minutes.", remain + 1);
        return false;
    }

    char clean[13];
    if (!voucher_validate_code_format(code_str, clean)) {
        g_bad_tries[acc_idx]++;
        if (g_bad_tries[acc_idx] >= bank->config.voucher_bad_tries) {
            g_blocked_until[acc_idx] = now + 1800; /* 30 simulated minutes */
            fraud_record_failure(bank, account_id, "Excessive invalid voucher attempts");
            if (out_msg) snprintf(out_msg, msg_len, "Too many invalid voucher attempts. Locked for 30 minutes.");
        } else {
            if (out_msg) snprintf(out_msg, msg_len, "Invalid voucher code format or checksum");
        }
        return false;
    }

    uint8_t hash[HASH_SIZE];
    sha256_hash((const uint8_t *)clean, 12, hash);

    Voucher *target = NULL;
    for (size_t i = 0; i < bank->voucher_count; ++i) {
        if (memcmp(bank->vouchers[i].code_hash, hash, HASH_SIZE) == 0) {
            target = &bank->vouchers[i];
            break;
        }
    }

    if (!target) {
        g_bad_tries[acc_idx]++;
        if (out_msg) snprintf(out_msg, msg_len, "Voucher code not found");
        return false;
    }

    if (target->status == VOUCHER_EXPIRED || (target->expiry_day > 0 && bank->sim_day > target->expiry_day)) {
        target->status = VOUCHER_EXPIRED;
        if (out_msg) snprintf(out_msg, msg_len, "This voucher has expired");
        return false;
    }
    if (target->status == VOUCHER_USED_UP || target->total_uses >= target->max_total_uses) {
        target->status = VOUCHER_USED_UP;
        if (out_msg) snprintf(out_msg, msg_len, "This voucher has already reached maximum uses");
        return false;
    }
    if (target->status == VOUCHER_VOID) {
        if (out_msg) snprintf(out_msg, msg_len, "This voucher has been voided by administrator");
        return false;
    }

    int used = get_account_usage(target->id, account_id);
    if (used >= target->uses_per_account) {
        if (out_msg) snprintf(out_msg, msg_len, "You have already redeemed this voucher");
        return false;
    }

    /* Redeem action */
    g_bad_tries[acc_idx] = 0;
    target->total_uses++;
    if (target->total_uses >= target->max_total_uses) {
        target->status = VOUCHER_USED_UP;
    }
    record_account_usage(target->id, account_id);

    if (target->type == VOUCHER_CREDIT) {
        acc->balance += target->value;
        txn_record(bank, acc->id, TXN_VOUCHER, target->value, acc->balance, 0, "Voucher credit applied", NULL);
        if (out_msg) snprintf(out_msg, msg_len, "Successfully redeemed credit voucher for %s%.2f!",
                              bank->config.currency, (double)target->value / 100.0);
    } else if (target->type == VOUCHER_CASHBACK) {
        if (out_msg) snprintf(out_msg, msg_len, "Cashback voucher activated for your next transfer!");
    } else if (target->type == VOUCHER_FEE_WAIVER) {
        if (out_msg) snprintf(out_msg, msg_len, "Late fee waiver voucher activated!");
    }

    audit_record(bank, "data", "CUSTOMER", "VOUCHER_REDEEM", "-", "Redeemed promotional voucher");
    return true;
}

void voucher_check_expiry(Bank *bank) {
    if (!bank) return;
    for (size_t i = 0; i < bank->voucher_count; ++i) {
        Voucher *v = &bank->vouchers[i];
        if (v->status == VOUCHER_ACTIVE && v->expiry_day > 0 && bank->sim_day > v->expiry_day) {
            v->status = VOUCHER_EXPIRED;
        }
    }
}
