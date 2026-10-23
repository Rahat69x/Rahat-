/**
 * include/vouchers.h
 * Promotional and credit voucher management (F6).
 */

#ifndef VOUCHERS_H
#define VOUCHERS_H

#include "types.h"
#include <stdbool.h>

#define VOUCHER_ALPHABET "0123456789ABCDEFGHJKMNPQRSTVWXYZ"

bool voucher_validate_code_format(const char *code_str, char *out_clean_12);

bool voucher_generate_code(char *out_formatted_15);

bool voucher_create(Bank *bank, const char *admin_id, VoucherType type, int64_t value,
                    int expiry_day, int max_uses, int uses_per_acc, char *out_code_str);

bool voucher_redeem(Bank *bank, long account_id, const char *code_str,
                    char *out_msg, size_t msg_len);

void voucher_check_expiry(Bank *bank);

#endif /* VOUCHERS_H */
