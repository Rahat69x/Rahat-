#ifndef HEAP_H
#define HEAP_H

#include "common.h"
#include "request.h"

/*
 * heap.h - Binary Max-Heap priority queue for emergency blood requests.
 * Layer 3 structure module: does NOT own Donor memory.
 * Priority rule:
 *  1. Urgency level: CRITICAL (2) > URGENT (1) > NORMAL (0)
 *  2. Tie-break: earlier arrival sequence (lower arrival_sequence number) has priority.
 * Never includes context.h.
 */

typedef struct RequestHeap {
    BloodRequest elements[MAX_PENDING_REQUESTS]; /* Array representation of binary heap */
    int size;                                    /* Current number of elements in heap */
} RequestHeap;

/*
 * Initializes an empty RequestHeap.
 * Time: O(1), Space: O(1)
 */
void heap_init(RequestHeap *h);

/*
 * Inserts a blood request into the heap (bubble up).
 * Big-O: Time O(log n), Space: O(1).
 * Returns ST_OK on success, ST_ERR_LIMIT if heap is full, ST_ERR_INVALID on bad args.
 */
Status heap_insert(RequestHeap *h, const BloodRequest *req);

/*
 * Extracts the highest-priority request from the heap root (bubble down).
 * Big-O: Time O(log n), Space: O(1).
 * Returns ST_OK on success, ST_ERR_EMPTY if heap is empty.
 */
Status heap_extract_max(RequestHeap *h, BloodRequest *out_req);

/*
 * Peeks at the highest-priority request at root without extracting it.
 * Big-O: Time O(1), Space: O(1).
 * Returns ST_OK on success, ST_ERR_EMPTY if heap is empty.
 */
Status heap_peek(const RequestHeap *h, BloodRequest *out_req);

/*
 * Returns 1 if heap is empty, 0 otherwise.
 * Time: O(1), Space: O(1)
 */
int heap_is_empty(const RequestHeap *h);

/*
 * Returns the current element count in the heap.
 * Time: O(1), Space: O(1)
 */
int heap_size(const RequestHeap *h);

#endif /* HEAP_H */
