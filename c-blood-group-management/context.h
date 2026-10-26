#ifndef CONTEXT_H
#define CONTEXT_H

#include "common.h"
#include "donor.h"

/*
 * context.h - Master SystemContext definition and life-cycle management.
 * Layer 4 module: central shared context passed by pointer to service modules.
 * Structure modules (donor, bst, hashtable, etc.) never include context.h.
 */

typedef struct SystemContext {
    Donor *donor_list_head; /* Head of master singly linked list (owner of Donor memory) */
    Donor *donor_list_tail; /* Tail of master singly linked list */
    int donor_count;        /* Active donor count */
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
