#include "context.h"

/*
 * context.c - Implementation of SystemContext initialization and teardown.
 */

/* Time: O(HASH_SIZE), Space: O(1) */
void context_init(SystemContext *ctx) {
    if (!ctx) return;
    ctx->donor_list_head = NULL;
    ctx->donor_list_tail = NULL;
    ctx->donor_id_bst = NULL;
    ht_init(&ctx->phone_table);
    ctx->donor_count = 0;
    ctx->next_donor_id = 1;
    ctx->next_request_id = 1;
    ctx->next_sequence = 1;
    ctx->fulfilled_count = 0;
}

/* Time: O(HASH_SIZE + n), Space: O(h) where n is total donors */
void context_destroy(SystemContext *ctx) {
    if (!ctx) return;
    /* 1. Tear down index structures first (they do NOT own donor memory) */
    bst_free_all(&ctx->donor_id_bst);
    ht_free_all(&ctx->phone_table);

    /* 2. Deallocate all Donors via master singly linked list (owner of memory) */
    donor_list_free_all(&ctx->donor_list_head, &ctx->donor_list_tail);

    ctx->donor_count = 0;
    ctx->next_donor_id = 1;
    ctx->next_request_id = 1;
    ctx->next_sequence = 1;
    ctx->fulfilled_count = 0;
}
