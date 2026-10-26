#include "heap.h"

/*
 * heap.c - Implementation of binary max-heap priority queue.
 */

/*
 * Returns > 0 if request 'a' has higher priority than request 'b'.
 * Returns < 0 if 'b' has higher priority than 'a'.
 * Time: O(1), Space: O(1)
 */
static int heap_compare(const BloodRequest *a, const BloodRequest *b) {
    if (a->urgency != b->urgency) {
        return (int)a->urgency - (int)b->urgency;
    }
    /* Tie-breaker: earlier arrival (smaller sequence number) has higher priority */
    return b->arrival_sequence - a->arrival_sequence;
}

/* Time: O(1), Space: O(1) */
static void heap_swap(BloodRequest *a, BloodRequest *b) {
    BloodRequest temp = *a;
    *a = *b;
    *b = temp;
}

/* Time: O(1), Space: O(1) */
void heap_init(RequestHeap *h) {
    if (!h) return;
    h->size = 0;
}

/* Time: O(log n), Space: O(1) */
Status heap_insert(RequestHeap *h, const BloodRequest *req) {
    if (!h || !req) {
        return ST_ERR_INVALID;
    }

    if (h->size >= MAX_PENDING_REQUESTS) {
        return ST_ERR_LIMIT;
    }

    /* 1. Append to end of heap array */
    int idx = h->size;
    h->elements[idx] = *req;
    h->size++;

    /* 2. Bubble up */
    while (idx > 0) {
        int parent = (idx - 1) / 2;
        if (heap_compare(&h->elements[idx], &h->elements[parent]) > 0) {
            heap_swap(&h->elements[idx], &h->elements[parent]);
            idx = parent;
        } else {
            break;
        }
    }

    return ST_OK;
}

/* Time: O(log n), Space: O(1) */
Status heap_extract_max(RequestHeap *h, BloodRequest *out_req) {
    if (!h || !out_req) return ST_ERR_INVALID;
    if (h->size <= 0) {
        return ST_ERR_EMPTY;
    }

    /* 1. Root holds maximum priority element */
    *out_req = h->elements[0];

    /* 2. Move last element to root and decrement size */
    h->size--;
    if (h->size > 0) {
        h->elements[0] = h->elements[h->size];

        /* 3. Bubble down */
        int idx = 0;
        for (;;) {
            int left = 2 * idx + 1;
            int right = 2 * idx + 2;
            int largest = idx;

            if (left < h->size && heap_compare(&h->elements[left], &h->elements[largest]) > 0) {
                largest = left;
            }
            if (right < h->size && heap_compare(&h->elements[right], &h->elements[largest]) > 0) {
                largest = right;
            }

            if (largest != idx) {
                heap_swap(&h->elements[idx], &h->elements[largest]);
                idx = largest;
            } else {
                break;
            }
        }
    }

    return ST_OK;
}

/* Time: O(1), Space: O(1) */
Status heap_peek(const RequestHeap *h, BloodRequest *out_req) {
    if (!h || !out_req) return ST_ERR_INVALID;
    if (h->size <= 0) {
        return ST_ERR_EMPTY;
    }
    *out_req = h->elements[0];
    return ST_OK;
}

/* Time: O(1), Space: O(1) */
int heap_is_empty(const RequestHeap *h) {
    return (!h || h->size == 0);
}

/* Time: O(1), Space: O(1) */
int heap_size(const RequestHeap *h) {
    if (!h) return 0;
    return h->size;
}
