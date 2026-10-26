#ifndef REQUEST_SERVICE_H
#define REQUEST_SERVICE_H

#include "context.h"
#include "request.h"
#include "queue.h"
#include "heap.h"
#include "undo.h"
#include "donor.h"

/*
 * request_service.h - Transactional blood request submission, prioritization,
 * donor matching, fulfillment processing, and undo.
 * Layer 5 module: coordinates request domain, queues, buckets, and donor entities.
 */

typedef struct FulfillmentDetails {
    BloodRequest request;                /* Fulfilled blood request */
    int matched_count;                   /* Number of donors matched (= request.units) */
    Donor *matched_donors[MAX_UNITS];    /* Pointers to matched donors */
    Date prev_donations[MAX_UNITS];      /* Previous last_donation dates for undo support */
    int was_emergency;                   /* 1 if dispatched from emergency heap, 0 if normal queue */
} FulfillmentDetails;

/*
 * Submits a new blood request.
 * Automatically routes to emergency heap (CRITICAL/URGENT) or normal FIFO queue (NORMAL).
 * Returns ST_OK on success, ST_ERR_LIMIT if capacity exceeded, ST_ERR_INVALID on bad input.
 */
Status request_service_add(SystemContext *ctx, const char *patient_name, BloodGroup bg,
                           int units, const char *hospital, const char *city,
                           UrgencyLevel urgency, Date today, int *out_id);

/*
 * Processes the next pending request by priority (Emergency Heap first, then Normal Queue).
 * Matches compatible eligible donors (exact blood group prioritized first).
 * Transactional prepare/commit: if insufficient donors are available, request is not dequeued
 * and donor records remain 100% untouched (returns ST_ERR_NO_DONORS).
 * If successful, pushes an UndoRecord onto ctx->undo_stack.
 */
Status request_service_process_next(SystemContext *ctx, Date today, FulfillmentDetails *out_details);

/*
 * Undoes the most recently processed blood request fulfillment.
 * Restores each donor's last_donation date to its prior date.
 * Re-enqueues the blood request back to where it originated (emergency heap or normal queue).
 * Decrements fulfilled_count.
 * Returns ST_OK on success, ST_ERR_EMPTY if no fulfillment records in undo stack.
 */
Status request_service_undo(SystemContext *ctx, BloodRequest *out_restored_req);

/*
 * Displays all currently pending blood requests (Emergency Heap + Normal Queue).
 */
void request_service_display_pending(const SystemContext *ctx);

#endif /* REQUEST_SERVICE_H */
