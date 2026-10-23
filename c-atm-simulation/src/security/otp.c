/**
 * src/security/otp.c
 * Simulated One-Time Password (OTP) subsystem (F1).
 */

#include "otp.h"
#include "rng.h"
#include "pin.h"
#include "clock.h"
#include "audit.h"
#include "fraud.h"
#include <stdio.h>
#include <string.h>

#define MAX_OTP_SESSIONS 32
static OtpSession g_otp_sessions[MAX_OTP_SESSIONS];

void otp_init(void) {
    memset(g_otp_sessions, 0, sizeof(g_otp_sessions));
}

static OtpSession *otp_find_session(long account_id, bool active_only) {
    for (int i = 0; i < MAX_OTP_SESSIONS; ++i) {
        if (g_otp_sessions[i].account_id == account_id) {
            if (!active_only || g_otp_sessions[i].is_active) {
                return &g_otp_sessions[i];
            }
        }
    }
    return NULL;
}

static OtpSession *otp_alloc_session(long account_id) {
    OtpSession *s = otp_find_session(account_id, false);
    if (s) return s;
    for (int i = 0; i < MAX_OTP_SESSIONS; ++i) {
        if (!g_otp_sessions[i].is_active) {
            return &g_otp_sessions[i];
        }
    }
    return &g_otp_sessions[0];
}

static void otp_print_simulated_sms(long account_id, const char *code, int ttl_s) {
    printf("\n+-------------------------------------------------------------+\n");
    printf("| [SIMULATED SMS] To: Customer (Account: ****%04ld)            |\n", account_id % 10000);
    printf("| Verification Code: %s                                   |\n", code);
    printf("| Valid for %d seconds. Do NOT share this code with anyone.   |\n", ttl_s);
    printf("+-------------------------------------------------------------+\n\n");
}

bool otp_issue(Bank *bank, long account_id, OtpPurpose purpose, int64_t amount,
               char *out_code_for_test) {
    if (!bank) return false;
    OtpSession *s = otp_alloc_session(account_id);
    memset(s, 0, sizeof(OtpSession));

    s->account_id = account_id;
    s->purpose = purpose;
    s->amount = amount;
    s->tries_left = bank->config.otp_max_tries;
    s->resends_left = bank->config.otp_resend_max;
    s->created_time = clock_now();
    s->last_send_time = s->created_time;
    s->is_active = true;

    uint32_t num = rng_u32() % 1000000;
    char code_str[8];
    snprintf(code_str, sizeof(code_str), "%06u", num);

    pin_generate_salt(s->salt);
    pin_hash_derive(code_str, s->salt, 1000, s->hash);

    if (out_code_for_test) {
        memcpy(out_code_for_test, code_str, 7);
    }

    otp_print_simulated_sms(account_id, code_str, bank->config.otp_ttl_s);
    audit_record(bank, "data", "CUSTOMER", "OTP_ISSUED", "-", "Simulated SMS dispatched");
    return true;
}

bool otp_verify(Bank *bank, long account_id, OtpPurpose purpose, int64_t amount,
                const char *entered_code, char *err_msg, size_t err_len) {
    if (!bank || !entered_code) return false;
    OtpSession *s = otp_find_session(account_id, true);
    if (!s || s->purpose != purpose || s->amount != amount) {
        if (err_msg) snprintf(err_msg, err_len, "No active verification session found");
        return false;
    }

    time_t now = clock_now();
    if (now > s->created_time + bank->config.otp_ttl_s) {
        s->is_active = false;
        if (err_msg) snprintf(err_msg, err_len, "Verification code has expired");
        audit_record(bank, "data", "CUSTOMER", "OTP_FAILED", "-", "Code expired");
        return false;
    }

    if (s->tries_left <= 0) {
        s->is_active = false;
        fraud_record_failure(bank, account_id, "OTP tries exhausted");
        if (err_msg) snprintf(err_msg, err_len, "Maximum verification attempts exceeded");
        audit_record(bank, "data", "CUSTOMER", "OTP_FAILED", "-", "Exceeded tries");
        return false;
    }

    if (!pin_verify(entered_code, s->salt, 1000, s->hash)) {
        s->tries_left--;
        fraud_record_failure(bank, account_id, "Invalid OTP code");
        audit_record(bank, "data", "CUSTOMER", "OTP_FAILED", "-", "Invalid code");
        if (s->tries_left <= 0) {
            s->is_active = false;
            if (err_msg) snprintf(err_msg, err_len, "Verification attempts exhausted. Operation cancelled.");
        } else {
            if (err_msg) snprintf(err_msg, err_len, "Invalid code. %d attempts remaining.", s->tries_left);
        }
        return false;
    }

    /* Success: single-use, deactivate immediately */
    s->is_active = false;
    audit_record(bank, "data", "CUSTOMER", "OTP_VERIFIED", "-", "Verification successful");
    return true;
}

bool otp_resend(Bank *bank, long account_id, OtpPurpose purpose, int64_t amount,
                char *out_code_for_test, char *err_msg, size_t err_len) {
    if (!bank) return false;
    OtpSession *s = otp_find_session(account_id, true);
    if (!s || s->purpose != purpose || s->amount != amount) {
        if (err_msg) snprintf(err_msg, err_len, "No active session to resend code for");
        return false;
    }

    if (s->resends_left <= 0) {
        if (err_msg) snprintf(err_msg, err_len, "Maximum code resends reached");
        return false;
    }

    time_t now = clock_now();
    if (now < s->last_send_time + bank->config.otp_resend_gap_s) {
        int wait_sec = (int)((s->last_send_time + bank->config.otp_resend_gap_s) - now);
        if (err_msg) snprintf(err_msg, err_len, "Please wait %d seconds before requesting another code", wait_sec);
        return false;
    }

    s->resends_left--;
    s->last_send_time = now;
    s->created_time = now;

    uint32_t num = rng_u32() % 1000000;
    char code_str[8];
    snprintf(code_str, sizeof(code_str), "%06u", num);

    pin_generate_salt(s->salt);
    pin_hash_derive(code_str, s->salt, 1000, s->hash);

    if (out_code_for_test) {
        memcpy(out_code_for_test, code_str, 7);
    }

    otp_print_simulated_sms(account_id, code_str, bank->config.otp_ttl_s);
    audit_record(bank, "data", "CUSTOMER", "OTP_ISSUED", "-", "Simulated SMS re-dispatched");
    return true;
}

void otp_cancel(long account_id) {
    OtpSession *s = otp_find_session(account_id, true);
    if (s) s->is_active = false;
}
