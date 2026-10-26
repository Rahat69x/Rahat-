#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"
#include "request.h"

/*
 * queue.h - FIFO linked queue for normal blood requests.
 * Layer 3 structure module: manages queue nodes dynamically.
 * Preserves strict arrival order (FIFO: First-In, First-Out).
 * Never includes context.h.
 */

typedef struct QueueNode {
    BloodRequest request;       /* Blood request record */
    struct QueueNode *next;     /* Pointer to next node in FIFO queue */
} QueueNode;

typedef struct RequestQueue {
    QueueNode *front;           /* Pointer to front node (next to be dequeued) */
    QueueNode *rear;            /* Pointer to rear node (last arrived) */
    int count;                  /* Total pending requests currently in queue */
} RequestQueue;

/*
 * Initializes an empty RequestQueue.
 * Time: O(1), Space: O(1)
 */
void queue_init(RequestQueue *q);

/*
 * Enqueues a request at the rear of the queue.
 * Preallocates QueueNode. Returns ST_OK on success, ST_ERR_NOMEM if malloc fails,
 * ST_ERR_LIMIT if MAX_PENDING_REQUESTS reached, ST_ERR_INVALID on bad arguments.
 * Time: O(1), Space: O(1) heap allocation.
 */
Status queue_enqueue(RequestQueue *q, const BloodRequest *req);

/*
 * Dequeues the request at the front of the queue into *out_req and frees the node.
 * Returns ST_OK on success, ST_ERR_EMPTY if queue is empty.
 * Time: O(1), Space: O(1).
 */
Status queue_dequeue(RequestQueue *q, BloodRequest *out_req);

/*
 * Peeks at the request at the front without removing it.
 * Returns ST_OK on success, ST_ERR_EMPTY if queue is empty.
 * Time: O(1), Space: O(1).
 */
Status queue_peek(const RequestQueue *q, BloodRequest *out_req);

/*
 * Returns 1 if queue is empty, 0 otherwise.
 * Time: O(1), Space: O(1)
 */
int queue_is_empty(const RequestQueue *q);

/*
 * Deallocates all nodes in the queue and sets front/rear to NULL, count to 0.
 * Time: O(n), Space: O(1).
 */
void queue_free_all(RequestQueue *q);

#endif /* QUEUE_H */
