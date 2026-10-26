#include "common.h"
#include "date.h"
#include "bloodgroup.h"
#include "utils.h"
#include "donor.h"
#include "bst.h"
#include "hashtable.h"
#include "buckets.h"
#include "request.h"
#include "queue.h"
#include "heap.h"
#include "undo.h"
#include "context.h"
#include "donor_service.h"
#include "request_service.h"
#include "testing.h"

/*
 * tests/test_undo.c - Automated test suite for Phase 8 (Undo Stack & Reversal).
 */

static void test_undo_stack_basic_operations(void) {
    TEST_INIT("Undo Stack Bounded Operations");

    UndoStack s;
    undo_init(&s);

    CHECK_EQ_INT(undo_is_empty(&s), 1);
    CHECK_EQ_INT(undo_count(&s), 0);

    BloodRequest r1;
    Date d = date_create(2026, 4, 15);
    request_init(&r1, 1, "Patient", BG_A_POS, 1, "Hospital", "Dhaka", URG_NORMAL, 1, d);

    UndoRecord rec;
    rec.request = r1;
    rec.matched_count = 1;
    rec.matched_donor_ids[0] = 5;
    rec.prev_donations[0] = date_never();
    rec.was_emergency = 0;

    /* Push up to UNDO_DEPTH (20) */
    for (int i = 0; i < UNDO_DEPTH; i++) {
        rec.matched_donor_ids[0] = i + 1;
        CHECK_EQ_INT(undo_push(&s, &rec), ST_OK);
    }
    CHECK_EQ_INT(undo_count(&s), UNDO_DEPTH);

    /* Push 21st: must evict index 0 (oldest) */
    rec.matched_donor_ids[0] = 999;
    CHECK_EQ_INT(undo_push(&s, &rec), ST_OK);
    CHECK_EQ_INT(undo_count(&s), UNDO_DEPTH);

    /* Pop top must be 999 */
    UndoRecord out;
    CHECK_EQ_INT(undo_pop(&s, &out), ST_OK);
    CHECK_EQ_INT(out.matched_donor_ids[0], 999);
    CHECK_EQ_INT(undo_count(&s), UNDO_DEPTH - 1);
}

static void test_request_undo_reversal(void) {
    TEST_INIT("End-to-End Request Undo Reversal");
    Date today = date_create(2026, 4, 15);

    SystemContext ctx;
    context_init(&ctx);

    /* 1. Register donor */
    int d_id = 0;
    Date prev_date = date_create(2026, 1, 1);
    donor_service_add(&ctx, "Donor One", 25, 'M', BG_O_POS, "01700000001", "Dhaka", prev_date, 1, today, &d_id);

    /* 2. Submit Emergency Request */
    int req_id = 0;
    request_service_add(&ctx, "Patient Critical", BG_O_POS, 1, "Emergency Hospital", "Dhaka", URG_CRITICAL, today, &req_id);
    CHECK_EQ_INT(heap_size(&ctx.emergency_heap), 1);
    CHECK_EQ_INT(ctx.fulfilled_count, 0);

    /* 3. Process fulfillment */
    FulfillmentDetails details;
    CHECK_EQ_INT(request_service_process_next(&ctx, today, &details), ST_OK);
    CHECK_EQ_INT(heap_size(&ctx.emergency_heap), 0);
    CHECK_EQ_INT(ctx.fulfilled_count, 1);
    CHECK_EQ_INT(undo_count(&ctx.undo_stack), 1);

    /* Verify donor date was updated to today */
    Donor *d = NULL;
    donor_service_find_by_id(&ctx, d_id, &d);
    CHECK(d != NULL);
    CHECK_EQ_INT(date_compare(d->last_donation, today), 0);

    /* 4. Execute Undo */
    BloodRequest restored_req;
    CHECK_EQ_INT(request_service_undo(&ctx, &restored_req), ST_OK);
    CHECK_EQ_INT(restored_req.id, req_id);
    CHECK_EQ_INT(ctx.fulfilled_count, 0);
    CHECK_EQ_INT(undo_count(&ctx.undo_stack), 0);

    /* Verify donor date was restored to prev_date */
    donor_service_find_by_id(&ctx, d_id, &d);
    CHECK(d != NULL);
    CHECK_EQ_INT(date_compare(d->last_donation, prev_date), 0);

    /* Verify request was re-inserted into emergency heap */
    CHECK_EQ_INT(heap_size(&ctx.emergency_heap), 1);

    /* 5. Second undo must fail with ST_ERR_EMPTY */
    CHECK_EQ_INT(request_service_undo(&ctx, &restored_req), ST_ERR_EMPTY);

    context_destroy(&ctx);
}

int main(void) {
    printf("========================================================\n");
    printf("     RUNNING PHASE 8 UNDO STACK AUTOMATED TESTS         \n");
    printf("========================================================\n");

    test_undo_stack_basic_operations();
    test_request_undo_reversal();

    TEST_REPORT();

    return (g_tests_failed == 0) ? 0 : 1;
}
