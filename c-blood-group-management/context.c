#include "context.h"

/*
 * context.c - Implementation of SystemContext initialization and teardown.
 */

/* Time: O(1), Space: O(1) */
void context_init(SystemContext *ctx) {
    if (!ctx) return;
    ctx->next_donor_id = 1;
    ctx->next_request_id = 1;
    ctx->next_sequence = 1;
    ctx->fulfilled_count = 0;
}

/* Time: O(1), Space: O(1) */
void context_destroy(SystemContext *ctx) {
    if (!ctx) return;
    /* In Phase 0, there are no dynamic structures to free.
     * Future phases will deallocate list, BST, hash table, queues, etc. here. */
    ctx->next_donor_id = 1;
    ctx->next_request_id = 1;
    ctx->next_sequence = 1;
    ctx->fulfilled_count = 0;
}
