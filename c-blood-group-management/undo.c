#include "undo.h"

/*
 * undo.c - Implementation of bounded LIFO stack for undoing fulfillments.
 */

/* Time: O(1), Space: O(1) */
void undo_init(UndoStack *s) {
    if (!s) return;
    s->top = -1;
}

/* Time: O(UNDO_DEPTH), Space: O(1) */
Status undo_push(UndoStack *s, const UndoRecord *rec) {
    if (!s || !rec) {
        return ST_ERR_INVALID;
    }

    if (s->top >= UNDO_DEPTH - 1) {
        /* Stack is full: evict oldest entry at index 0 by shifting left */
        for (int i = 0; i < UNDO_DEPTH - 1; i++) {
            s->entries[i] = s->entries[i + 1];
        }
        s->entries[UNDO_DEPTH - 1] = *rec;
        s->top = UNDO_DEPTH - 1;
    } else {
        s->top++;
        s->entries[s->top] = *rec;
    }

    return ST_OK;
}

/* Time: O(1), Space: O(1) */
Status undo_pop(UndoStack *s, UndoRecord *out_rec) {
    if (!s || !out_rec) return ST_ERR_INVALID;
    if (s->top < 0) {
        return ST_ERR_EMPTY;
    }

    *out_rec = s->entries[s->top];
    s->top--;
    return ST_OK;
}

/* Time: O(1), Space: O(1) */
Status undo_peek(const UndoStack *s, UndoRecord *out_rec) {
    if (!s || !out_rec) return ST_ERR_INVALID;
    if (s->top < 0) {
        return ST_ERR_EMPTY;
    }

    *out_rec = s->entries[s->top];
    return ST_OK;
}

/* Time: O(1), Space: O(1) */
int undo_is_empty(const UndoStack *s) {
    return (!s || s->top < 0);
}

/* Time: O(1), Space: O(1) */
int undo_count(const UndoStack *s) {
    if (!s || s->top < 0) return 0;
    return s->top + 1;
}
