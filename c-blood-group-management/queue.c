#include "queue.h"

/*
 * queue.c - Implementation of linked FIFO queue for normal blood requests.
 */

/* Time: O(1), Space: O(1) */
void queue_init(RequestQueue *q) {
    if (!q) return;
    q->front = NULL;
    q->rear = NULL;
    q->count = 0;
}

/* Time: O(1), Space: O(1) heap allocation */
Status queue_enqueue(RequestQueue *q, const BloodRequest *req) {
    if (!q || !req) {
        return ST_ERR_INVALID;
    }

    if (q->count >= MAX_PENDING_REQUESTS) {
        return ST_ERR_LIMIT;
    }

    /* 1. Prepare phase: allocate QueueNode */
    QueueNode *node = (QueueNode *)malloc(sizeof(QueueNode));
    if (!node) {
        return ST_ERR_NOMEM;
    }

    node->request = *req;
    node->next = NULL;

    /* 2. Commit phase: append to rear of queue */
    if (q->rear == NULL) {
        q->front = node;
        q->rear = node;
    } else {
        q->rear->next = node;
        q->rear = node;
    }
    q->count++;

    return ST_OK;
}

/* Time: O(1), Space: O(1) */
Status queue_dequeue(RequestQueue *q, BloodRequest *out_req) {
    if (!q || !out_req) return ST_ERR_INVALID;
    if (q->front == NULL || q->count == 0) {
        return ST_ERR_EMPTY;
    }

    QueueNode *front_node = q->front;
    *out_req = front_node->request;

    q->front = front_node->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }
    q->count--;

    free(front_node);
    return ST_OK;
}

/* Time: O(1), Space: O(1) */
Status queue_peek(const RequestQueue *q, BloodRequest *out_req) {
    if (!q || !out_req) return ST_ERR_INVALID;
    if (q->front == NULL || q->count == 0) {
        return ST_ERR_EMPTY;
    }
    *out_req = q->front->request;
    return ST_OK;
}

/* Time: O(1), Space: O(1) */
int queue_is_empty(const RequestQueue *q) {
    return (!q || q->count == 0 || q->front == NULL);
}

/* Time: O(n), Space: O(1) */
void queue_free_all(RequestQueue *q) {
    if (!q) return;
    QueueNode *curr = q->front;
    while (curr) {
        QueueNode *next_node = curr->next;
        free(curr);
        curr = next_node;
    }
    q->front = NULL;
    q->rear = NULL;
    q->count = 0;
}
