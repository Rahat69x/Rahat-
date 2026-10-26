#include "request_service.h"
#include "donor_service.h"

/*
 * request_service.c - Implementation of request submission, matching algorithm, and undo.
 */

/* Time: O(log n) for emergency or O(1) for normal, Space: O(1) */
Status request_service_add(SystemContext *ctx, const char *patient_name, BloodGroup bg,
                           int units, const char *hospital, const char *city,
                           UrgencyLevel urgency, Date today, int *out_id) {
    if (!ctx) return ST_ERR_INVALID;

    /* 1. Field validation */
    Status st = request_validate_fields(patient_name, bg, units, hospital, city, urgency);
    if (st != ST_OK) {
        return st;
    }

    /* 2. Initialize request object */
    BloodRequest req;
    request_init(&req, ctx->next_request_id, patient_name, bg, units, hospital, city,
                 urgency, ctx->next_sequence, today);

    /* 3. Route according to urgency */
    if (urgency == URG_CRITICAL || urgency == URG_URGENT) {
        st = heap_insert(&ctx->emergency_heap, &req);
        if (st != ST_OK) {
            return st;
        }
    } else {
        st = queue_enqueue(&ctx->normal_queue, &req);
        if (st != ST_OK) {
            return st;
        }
    }

    if (out_id) {
        *out_id = ctx->next_request_id;
    }
    ctx->next_request_id++;
    ctx->next_sequence++;

    return ST_OK;
}

/* Time: O(units * donors_in_buckets), Space: O(units) */
Status request_service_process_next(SystemContext *ctx, Date today, FulfillmentDetails *out_details) {
    if (!ctx || !out_details) return ST_ERR_INVALID;

    /* 1. Identify highest-priority pending request (Emergency Heap first, then Normal Queue) */
    BloodRequest req;
    int was_emergency = 0;

    if (!heap_is_empty(&ctx->emergency_heap)) {
        Status st = heap_peek(&ctx->emergency_heap, &req);
        if (st != ST_OK) return st;
        was_emergency = 1;
    } else if (!queue_is_empty(&ctx->normal_queue)) {
        Status st = queue_peek(&ctx->normal_queue, &req);
        if (st != ST_OK) return st;
        was_emergency = 0;
    } else {
        return ST_ERR_EMPTY; /* No pending requests */
    }

    /* 2. Prepare Phase: Match compatible eligible donors */
    BloodGroup comp_groups[BG_COUNT];
    int comp_count = bloodgroup_get_compatible_donors(req.blood_group, comp_groups);

    Donor *candidates[MAX_UNITS];
    Date prev_dates[MAX_UNITS];
    int matched = 0;

    for (int g = 0; g < comp_count && matched < req.units; g++) {
        BloodGroup target_bg = comp_groups[g];
        const BucketNode *curr = buckets_get_head(&ctx->bg_buckets, target_bg);

        while (curr && matched < req.units) {
            Donor *d = curr->donor;

            /* Check availability and 90-day cooldown eligibility */
            if (bloodgroup_is_eligible(d->available, d->last_donation, today)) {
                /* Check not already selected */
                int already_picked = 0;
                for (int i = 0; i < matched; i++) {
                    if (candidates[i] == d) {
                        already_picked = 1;
                        break;
                    }
                }

                if (!already_picked) {
                    candidates[matched] = d;
                    prev_dates[matched] = d->last_donation;
                    matched++;
                }
            }
            curr = curr->next;
        }
    }

    /* 3. Transactional check: insufficient donors available */
    if (matched < req.units) {
        /* State 100% untouched. Request remains at front of queue/heap */
        return ST_ERR_NO_DONORS;
    }

    /* 4. Commit Phase: Dequeue request and update donor records */
    BloodRequest extracted;
    if (was_emergency) {
        heap_extract_max(&ctx->emergency_heap, &extracted);
    } else {
        queue_dequeue(&ctx->normal_queue, &extracted);
    }

    out_details->request = extracted;
    out_details->matched_count = matched;
    out_details->was_emergency = was_emergency;

    for (int i = 0; i < matched; i++) {
        out_details->matched_donors[i] = candidates[i];
        out_details->prev_donations[i] = prev_dates[i];
        /* Update donor's last donation date to today */
        candidates[i]->last_donation = today;
    }

    /* 5. Push onto Undo Stack */
    UndoRecord undo_rec;
    undo_rec.request = extracted;
    undo_rec.matched_count = matched;
    undo_rec.was_emergency = was_emergency;
    for (int i = 0; i < matched; i++) {
        undo_rec.matched_donor_ids[i] = candidates[i]->id;
        undo_rec.prev_donations[i] = prev_dates[i];
    }
    undo_push(&ctx->undo_stack, &undo_rec);

    ctx->fulfilled_count++;
    return ST_OK;
}

/* Time: O(units * log(donors)), Space: O(1) */
Status request_service_undo(SystemContext *ctx, BloodRequest *out_restored_req) {
    if (!ctx) return ST_ERR_INVALID;

    UndoRecord rec;
    Status st = undo_pop(&ctx->undo_stack, &rec);
    if (st != ST_OK) {
        return st; /* ST_ERR_EMPTY if no undo operations available */
    }

    /* 1. Restore donors' previous donation dates */
    for (int i = 0; i < rec.matched_count; i++) {
        Donor *d = NULL;
        donor_service_find_by_id(ctx, rec.matched_donor_ids[i], &d);
        if (d) {
            d->last_donation = rec.prev_donations[i];
        }
    }

    /* 2. Re-insert request into originating queue/heap */
    if (rec.was_emergency) {
        heap_insert(&ctx->emergency_heap, &rec.request);
    } else {
        queue_enqueue(&ctx->normal_queue, &rec.request);
    }

    if (ctx->fulfilled_count > 0) {
        ctx->fulfilled_count--;
    }

    if (out_restored_req) {
        *out_restored_req = rec.request;
    }

    return ST_OK;
}

/* Time: O(heap_size + queue_size), Space: O(1) */
void request_service_display_pending(const SystemContext *ctx) {
    if (!ctx) return;

    printf("\n===================================================================================================================\n");
    printf("                                       PENDING BLOOD REQUESTS DIRECTORY                                            \n");
    printf("===================================================================================================================\n");

    /* 1. Emergency Priority Queue (Heap) */
    printf("\n--- EMERGENCY REQUESTS (PRIORITY HEAP: CRITICAL & URGENT) ---\n");
    int em_count = heap_size(&ctx->emergency_heap);
    if (em_count == 0) {
        printf("  [No emergency requests pending.]\n");
    } else {
        for (int i = 0; i < em_count; i++) {
            const BloodRequest *r = &ctx->emergency_heap.elements[i];
            char d_str[16];
            date_format(r->request_date, d_str, sizeof(d_str));
            printf("  ID: #%-4d | Patient: %-20s | Group: %-3s | Units: %2d | Urgency: %-8s | City: %-12s | Hospital: %s\n",
                   r->id, r->patient_name, bloodgroup_to_string(r->blood_group),
                   r->units, urgency_to_string(r->urgency), r->city, r->hospital);
        }
    }

    /* 2. Normal FIFO Queue */
    printf("\n--- NORMAL REQUESTS (FIFO QUEUE: FIRST-IN, FIRST-OUT) ---\n");
    if (queue_is_empty(&ctx->normal_queue)) {
        printf("  [No normal requests pending.]\n");
    } else {
        const QueueNode *curr = ctx->normal_queue.front;
        while (curr) {
            const BloodRequest *r = &curr->request;
            char d_str[16];
            date_format(r->request_date, d_str, sizeof(d_str));
            printf("  ID: #%-4d | Patient: %-20s | Group: %-3s | Units: %2d | Urgency: %-8s | City: %-12s | Hospital: %s\n",
                   r->id, r->patient_name, bloodgroup_to_string(r->blood_group),
                   r->units, urgency_to_string(r->urgency), r->city, r->hospital);
            curr = curr->next;
        }
    }

    printf("-------------------------------------------------------------------------------------------------------------------\n");
    printf("Summary: Emergency: %d | Normal: %d | Total Pending: %d | Total Fulfilled: %d\n",
           em_count, ctx->normal_queue.count, em_count + ctx->normal_queue.count, ctx->fulfilled_count);
    printf("===================================================================================================================\n");
}
