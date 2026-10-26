#ifndef UNDO_H
#define UNDO_H

#include "common.h"
#include "request.h"

/*
 * undo.h - Bounded LIFO stack for reversing blood request fulfillments.
 * Layer 3 structure module: does NOT own Donor memory.
 * Bounded by UNDO_DEPTH (20). When full, the oldest bottom entry is evicted.
 * Never includes context.h.
 */

typedef struct UndoRecord {
    BloodRequest request;                /* Request that was fulfilled */
    int matched_donor_ids[MAX_UNITS];    /* IDs of donors who donated for this request */
    Date prev_donations[MAX_UNITS];      /* Donor's previous last_donation date prior to fulfillment */
    int matched_count;                   /* Number of matched donors (1..MAX_UNITS) */
    int was_emergency;                   /* 1 if request was dispatched from emergency heap, 0 if queue */
} UndoRecord;

typedef struct UndoStack {
    UndoRecord entries[UNDO_DEPTH];      /* Fixed array representation of bounded stack */
    int top;                             /* Top element index (-1 when empty) */
} UndoStack;

/*
 * Initializes an empty UndoStack.
 * Time: O(1), Space: O(1)
 */
void undo_init(UndoStack *s);

/*
 * Pushes an UndoRecord onto the stack.
 * If the stack is full, shifts entries down to evict the oldest entry at bottom (O(UNDO_DEPTH)).
 * Time: O(UNDO_DEPTH), Space: O(1).
 */
Status undo_push(UndoStack *s, const UndoRecord *rec);

/*
 * Pops the top UndoRecord from the stack into *out_rec.
 * Returns ST_OK on success, ST_ERR_EMPTY if stack is empty.
 * Time: O(1), Space: O(1).
 */
Status undo_pop(UndoStack *s, UndoRecord *out_rec);

/*
 * Peeks at the top UndoRecord without popping it.
 * Returns ST_OK on success, ST_ERR_EMPTY if stack is empty.
 * Time: O(1), Space: O(1).
 */
Status undo_peek(const UndoStack *s, UndoRecord *out_rec);

/*
 * Returns 1 if stack is empty, 0 otherwise.
 * Time: O(1), Space: O(1)
 */
int undo_is_empty(const UndoStack *s);

/*
 * Returns current number of entries stored in the stack (0 to UNDO_DEPTH).
 * Time: O(1), Space: O(1)
 */
int undo_count(const UndoStack *s);

#endif /* UNDO_H */
