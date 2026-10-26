#include "context.h"

/*
 * context.c - Implementation of SystemContext initialization and teardown.
 */

/* Time: O(1), Space: O(1) */
void context_init(SystemContext *ctx) {
    if (!ctx) return;
    ctx->donor_list_head = NULL;
    ctx->donor_list_tail = NULL;
    ctx->donor_count = 0;
    ctx->next_donor_id = 1;
    ctx->next_request_id = 1;
    ctx->next_sequence = 1;
    ctx->fulfilled_count = 0;
}

/* Time: O(n), Space: O(1) where n is total donors */
void context_destroy(SystemContext *ctx) {
    if (!ctx) return;
    /* Deallocate all Donors via master singly linked list */
    donor_list_free_all(&ctx->donor_list_head, &ctx->donor_list_tail);
    ctx->donor_count = 0;
    ctx->next_donor_id = 1;
    ctx->next_request_id = 1;
    ctx->next_sequence = 1;
    ctx->fulfilled_count = 0;
}
