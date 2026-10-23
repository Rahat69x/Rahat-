/**
 * include/otp.h
 * Simulated One-Time Password (OTP) subsystem (F1).
 */

#ifndef OTP_H
#define OTP_H

#include "types.h"
#include <stdbool.h>

typedef enum {
    OTP_PURPOSE_PIN_CHANGE = 1,
    OTP_PURPOSE_TRANSFER = 2,
    OTP_PURPOSE_LOAN = 3,
    OTP_PURPOSE_STANDING_ORDER = 4,
    OTP_PURPOSE_STEPUP = 5
} OtpPurpose;

typedef struct {
    long account_id;
    OtpPurpose purpose;
    int64_t amount;
    uint8_t salt[SALT_SIZE];
    uint8_t hash[HASH_SIZE];
    time_t created_time;
    time_t last_send_time;
    int tries_left;
    int resends_left;
    bool is_active;
} OtpSession;

void otp_init(void);

bool otp_issue(Bank *bank, long account_id, OtpPurpose purpose, int64_t amount,
               char *out_code_for_test);

bool otp_verify(Bank *bank, long account_id, OtpPurpose purpose, int64_t amount,
                const char *entered_code, char *err_msg, size_t err_len);

bool otp_resend(Bank *bank, long account_id, OtpPurpose purpose, int64_t amount,
                char *out_code_for_test, char *err_msg, size_t err_len);

void otp_cancel(long account_id);

#endif /* OTP_H */
