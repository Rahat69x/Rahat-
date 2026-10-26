#ifndef CONTEXT_H
#define CONTEXT_H

#include "common.h"

/*
 * context.h - Master SystemContext definition and life-cycle management.
 * Layer 4 module: central shared context passed by pointer to service modules.
 * Structure modules (bst, hashtable, donor, etc.) never include context.h.
 *
 * In Phase 0, initializes the monotonic counters and fulfilled metrics.
 * As phases progress, structure pointers (list, bst, hashtable, etc.) are added.
 */

typedef struct SystemContext {
    int next_donor_id;      /* Monotonically increasing ID counter (never reused) */
    int next_request_id;    /* Request ID counter */
    int next_sequence;      /* Global arrival sequence counter for tie-breaking */
    int fulfilled_count;    /* Total fulfilled blood requests counter */
} SystemContext;

/* Initialize SystemContext to default baseline values */
void context_init(SystemContext *ctx);

/* Clean up and deallocate all components within SystemContext */
void context_destroy(SystemContext *ctx);

#endif /* CONTEXT_H */
