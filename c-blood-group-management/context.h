#ifndef CONTEXT_H
#define CONTEXT_H

#include "common.h"
#include "donor.h"
#include "bst.h"
#include "hashtable.h"
#include "buckets.h"
#include "request.h"
#include "queue.h"
#include "heap.h"
#include "undo.h"

/*
 * context.h - Master SystemContext definition and life-cycle management.
 * Layer 4 module: central shared context passed by pointer to service modules.
 * Structure modules never include context.h.
 */

typedef struct SystemContext {
    Donor *donor_list_head;       /* Head of master singly linked list (owner of Donor memory) */
    Donor *donor_list_tail;       /* Tail of master singly linked list */
    BSTNode *donor_id_bst;        /* Root of Binary Search Tree index by donor ID */
    HashTable phone_table;        /* Hash table index by donor phone number */
    BloodGroupBuckets bg_buckets; /* 8-group classified donor buckets index */
    RequestQueue normal_queue;    /* FIFO queue for normal blood requests */
    RequestHeap emergency_heap;   /* Max-heap priority queue for emergency requests */
    UndoStack undo_stack;         /* Bounded LIFO stack for reversing fulfillments */
    int donor_count;              /* Active donor count */
    int next_donor_id;            /* Monotonically increasing ID counter (never reused) */
    int next_request_id;          /* Request ID counter */
    int next_sequence;            /* Global arrival sequence counter for tie-breaking */
    int fulfilled_count;          /* Total fulfilled blood requests counter */
} SystemContext;

/* Initialize SystemContext to default baseline values */
void context_init(SystemContext *ctx);

/* Clean up and deallocate all components within SystemContext */
void context_destroy(SystemContext *ctx);

#endif /* CONTEXT_H */
