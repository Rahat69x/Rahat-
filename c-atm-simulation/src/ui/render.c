/**
 * src/ui/render.c
 * Formatted terminal rendering, masked accounts, and receipt printing (C5).
 */

#include "render.h"
#include "money.h"
#include <stdio.h>
#include <string.h>

void render_mask_account(long account_id, char *out, size_t out_len) {
    if (!out || out_len == 0) return;
    char raw[32];
    snprintf(raw, sizeof(raw), "%ld", account_id);
    size_t len = strlen(raw);
    if (len <= 4) {
        snprintf(out, out_len, "****%s", raw);
    } else {
        snprintf(out, out_len, "****%s", &raw[len - 4]);
    }
}

void render_receipt(const char *receipt_no, int day, time_t t, const char *type_str,
                    int64_t amount, int64_t balance_after, const char *currency,
                    long account_id, long counterparty) {
    char amt_buf[32], bal_buf[32], masked_acc[32], masked_party[32];
    money_format(amount, currency, amt_buf, sizeof(amt_buf));
    money_format(balance_after, currency, bal_buf, sizeof(bal_buf));
    render_mask_account(account_id, masked_acc, sizeof(masked_acc));

    printf("\n+--------------------------------------------------+\n");
    printf("|              ATM TRANSACTION RECEIPT             |\n");
    printf("+--------------------------------------------------+\n");
    printf("| Receipt No  : %-34s |\n", receipt_no);
    printf("| Day / Time  : Day %-3d | %-24ld |\n", day, (long)t);
    printf("| Operation   : %-34s |\n", type_str);
    printf("| Account     : %-34s |\n", masked_acc);
    if (counterparty > 0) {
        render_mask_account(counterparty, masked_party, sizeof(masked_party));
        printf("| Counterparty: %-34s |\n", masked_party);
    }
    printf("| Amount      : %-34s |\n", amt_buf);
    printf("| New Balance : %-34s |\n", bal_buf);
    printf("+--------------------------------------------------+\n\n");
}
