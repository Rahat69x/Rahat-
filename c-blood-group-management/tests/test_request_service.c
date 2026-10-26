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
#include "context.h"
#include "donor_service.h"
#include "request_service.h"
#include "testing.h"

/*
 * tests/test_request_service.c - Automated test suite for Phase 7 (Request & Donor Matching).
 */

static void test_request_routing_and_priority(void) {
    TEST_INIT("Request Submission & Priority Routing");
    Date today = date_create(2026, 4, 15);

    SystemContext ctx;
    context_init(&ctx);

    int id_norm = 0, id_urg = 0, id_crit = 0;
    CHECK_EQ_INT(request_service_add(&ctx, "Patient Normal", BG_A_POS, 1, "Hospital A", "Dhaka", URG_NORMAL, today, &id_norm), ST_OK);
    CHECK_EQ_INT(request_service_add(&ctx, "Patient Urgent", BG_B_POS, 1, "Hospital B", "Dhaka", URG_URGENT, today, &id_urg), ST_OK);
    CHECK_EQ_INT(request_service_add(&ctx, "Patient Critical", BG_O_POS, 1, "Hospital C", "Dhaka", URG_CRITICAL, today, &id_crit), ST_OK);

    CHECK_EQ_INT(ctx.normal_queue.count, 1);
    CHECK_EQ_INT(heap_size(&ctx.emergency_heap), 2);

    /* Priority check in emergency heap: root must be CRITICAL */
    BloodRequest em_top;
    CHECK_EQ_INT(heap_peek(&ctx.emergency_heap, &em_top), ST_OK);
    CHECK_EQ_INT(em_top.urgency, URG_CRITICAL);
    CHECK_EQ_INT(em_top.id, id_crit);

    context_destroy(&ctx);
}

static void test_matching_and_fulfillment(void) {
    TEST_INIT("Donor Matching Algorithm & Cooldown Rules");
    Date today = date_create(2026, 4, 15);

    SystemContext ctx;
    context_init(&ctx);

    /* Register donors:
     * d1: A+ (never donated, eligible)
     * d2: A- (donated 2026-04-01, only 14 days ago -> INELIGIBLE cooldown)
     * d3: O+ (donated 2026-01-01, >90 days ago -> ELIGIBLE)
     * d4: B+ (incompatible for A+ patient)
     */
    int d_id1 = 0, d_id2 = 0, d_id3 = 0, d_id4 = 0;
    donor_service_add(&ctx, "Donor A1", 25, 'M', BG_A_POS, "01700000001", "Dhaka", date_never(), 1, today, &d_id1);
    donor_service_add(&ctx, "Donor A2", 28, 'F', BG_A_NEG, "01700000002", "Dhaka", date_create(2026, 4, 1), 1, today, &d_id2);
    donor_service_add(&ctx, "Donor O1", 30, 'M', BG_O_POS, "01700000003", "Dhaka", date_create(2026, 1, 1), 1, today, &d_id3);
    donor_service_add(&ctx, "Donor B1", 32, 'M', BG_B_POS, "01700000004", "Dhaka", date_never(), 1, today, &d_id4);

    /* Submit Request: Patient needs 2 units of A+ */
    int req_id = 0;
    request_service_add(&ctx, "Patient Alice", BG_A_POS, 2, "Dhaka Hospital", "Dhaka", URG_NORMAL, today, &req_id);

    /* Process next request:
     * Needs 2 units.
     * Eligible compatible candidates: d1 (A+, exact match), d3 (O+, compatible).
     * d2 is skipped because of cooldown. d4 is skipped because incompatible.
     */
    FulfillmentDetails details;
    Status st = request_service_process_next(&ctx, today, &details);
    CHECK_EQ_INT(st, ST_OK);
    CHECK_EQ_INT(details.matched_count, 2);
    CHECK_EQ_INT(details.matched_donors[0]->id, d_id1); /* Exact match A+ */
    CHECK_EQ_INT(details.matched_donors[1]->id, d_id3); /* Compatible O+ */
    CHECK_EQ_INT(ctx.fulfilled_count, 1);
    CHECK_EQ_INT(ctx.normal_queue.count, 0);

    /* Both matched donors must have their last_donation updated to today */
    CHECK_EQ_INT(date_compare(details.matched_donors[0]->last_donation, today), 0);
    CHECK_EQ_INT(date_compare(details.matched_donors[1]->last_donation, today), 0);

    /* Submit another request for A+ (now both d1 and d3 are in cooldown!) */
    int req_id2 = 0;
    request_service_add(&ctx, "Patient Bob", BG_A_POS, 1, "Dhaka Hospital", "Dhaka", URG_NORMAL, today, &req_id2);

    /* Process must fail with ST_ERR_NO_DONORS */
    st = request_service_process_next(&ctx, today, &details);
    CHECK_EQ_INT(st, ST_ERR_NO_DONORS);
    /* Request remains in queue untouched! */
    CHECK_EQ_INT(ctx.normal_queue.count, 1);
    CHECK_EQ_INT(ctx.fulfilled_count, 1);

    context_destroy(&ctx);
}

int main(void) {
    printf("========================================================\n");
    printf("     RUNNING PHASE 7 REQUEST SERVICE AUTOMATED TESTS    \n");
    printf("========================================================\n");

    test_request_routing_and_priority();
    test_matching_and_fulfillment();

    TEST_REPORT();

    return (g_tests_failed == 0) ? 0 : 1;
}
