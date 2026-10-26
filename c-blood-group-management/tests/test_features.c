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
#include "citygraph.h"
#include "history.h"
#include "context.h"
#include "donor_service.h"
#include "request_service.h"
#include "testing.h"

/*
 * tests/test_features.c - Automated test suite for Feature F1 (City Graph) and F2 (Fulfilment History).
 */

static void test_feature_f1_citygraph(void) {
    TEST_INIT("Feature F1: City Graph & Dijkstra Shortest Paths");

    CityGraph g;
    citygraph_init(&g);

    /* Build a triangle network: Dhaka - Comilla - Chittagong */
    CHECK_EQ_INT(citygraph_add_road(&g, "Dhaka", "Comilla", 97), ST_OK);
    CHECK_EQ_INT(citygraph_add_road(&g, "Comilla", "Chittagong", 150), ST_OK);
    CHECK_EQ_INT(citygraph_add_road(&g, "Dhaka", "Mymensingh", 120), ST_OK);
    /* Add disconnected city */
    int isl_idx = citygraph_add_city(&g, "Sandwip-Island");
    CHECK(isl_idx >= 0);

    CHECK_EQ_INT(g.city_count, 5);

    int dhaka_idx = citygraph_find_city(&g, "Dhaka");
    int ctg_idx = citygraph_find_city(&g, "Chittagong");
    int mym_idx = citygraph_find_city(&g, "Mymensingh");
    CHECK(dhaka_idx >= 0);
    CHECK(ctg_idx >= 0);

    /* Run Dijkstra from Dhaka */
    int dist[MAX_CITIES];
    citygraph_dijkstra(&g, dhaka_idx, dist);

    CHECK_EQ_INT(dist[dhaka_idx], 0);
    CHECK_EQ_INT(dist[mym_idx], 120);
    CHECK_EQ_INT(dist[ctg_idx], 247); /* 97 + 150 */
    CHECK_EQ_INT(dist[isl_idx], INF_KM); /* Disconnected */

    citygraph_free(&g);
    CHECK_EQ_INT(g.city_count, 0);
}

static void test_feature_f2_history(void) {
    TEST_INIT("Feature F2: Fulfilment History Doubly Linked List");

    HistoryList h;
    history_init(&h);
    CHECK_EQ_INT(h.count, 0);
    CHECK(h.head == NULL && h.tail == NULL);

    FulfilmentRecord rec;
    rec.request_id = 1;
    utils_copy_string(rec.patient_name, "Patient 1", sizeof(rec.patient_name));
    rec.blood_group = BG_O_POS;
    rec.units = 1;
    utils_copy_string(rec.hospital, "Hospital", sizeof(rec.hospital));
    utils_copy_string(rec.patient_city, "Dhaka", sizeof(rec.patient_city));
    rec.fulfillment_date = date_create(2026, 4, 15);
    rec.donor_ids[0] = 10;
    utils_copy_string(rec.donor_names[0], "Donor 10", sizeof(rec.donor_names[0]));
    utils_copy_string(rec.donor_cities[0], "Dhaka", sizeof(rec.donor_cities[0]));
    rec.donor_distances_km[0] = 0;

    /* Append records */
    CHECK_EQ_INT(history_append(&h, &rec), ST_OK);
    CHECK_EQ_INT(h.count, 1);
    CHECK(h.head == h.tail);

    rec.request_id = 2;
    CHECK_EQ_INT(history_append(&h, &rec), ST_OK);
    CHECK_EQ_INT(h.count, 2);
    CHECK(h.head->next == h.tail);
    CHECK(h.tail->prev == h.head);

    /* Test remove tail on undo */
    FulfilmentRecord removed;
    CHECK_EQ_INT(history_remove_tail(&h, &removed), ST_OK);
    CHECK_EQ_INT(removed.request_id, 2);
    CHECK_EQ_INT(h.count, 1);
    CHECK(h.tail == h.head);

    history_free_all(&h);
    CHECK_EQ_INT(h.count, 0);
    CHECK(h.head == NULL && h.tail == NULL);
}

static void test_f1_and_f2_integrated_matching(void) {
    TEST_INIT("Integrated Dijkstra Nearest-Donor Ranking & History Logging");
    Date today = date_create(2026, 4, 15);

    SystemContext ctx;
    context_init(&ctx);

    /* Register three eligible O+ donors in different cities:
     * Donor A: Chittagong (247 km from Dhaka)
     * Donor B: Comilla (97 km from Dhaka) -> NEAREST!
     * Donor C: Sylhet (240 km from Dhaka)
     */
    int d_a = 0, d_b = 0, d_c = 0;
    donor_service_add(&ctx, "Donor Chittagong", 25, 'M', BG_O_POS, "01700000001", "Chittagong", date_never(), 1, today, &d_a);
    donor_service_add(&ctx, "Donor Comilla", 28, 'M', BG_O_POS, "01700000002", "Comilla", date_never(), 1, today, &d_b);
    donor_service_add(&ctx, "Donor Sylhet", 30, 'M', BG_O_POS, "01700000003", "Sylhet", date_never(), 1, today, &d_c);

    /* Submit emergency request for 1 unit in Dhaka */
    int req_id = 0;
    request_service_add(&ctx, "Emergency Patient", BG_O_POS, 1, "Dhaka Medical", "Dhaka", URG_CRITICAL, today, &req_id);

    /* Process request: Comilla donor (97 km) must be chosen because nearest! */
    FulfillmentDetails details;
    CHECK_EQ_INT(request_service_process_next(&ctx, today, &details), ST_OK);
    CHECK_EQ_INT(details.matched_count, 1);
    CHECK_EQ_INT(details.matched_donors[0]->id, d_b); /* Donor in Comilla selected! */

    /* Verify history entry was created with distance 97 km */
    CHECK_EQ_INT(ctx.history.count, 1);
    CHECK(ctx.history.tail != NULL);
    CHECK_EQ_INT(ctx.history.tail->record.donor_ids[0], d_b);
    CHECK_EQ_INT(ctx.history.tail->record.donor_distances_km[0], 97);

    /* Undo fulfillment: history tail removed, donor last_donation restored */
    BloodRequest restored;
    CHECK_EQ_INT(request_service_undo(&ctx, &restored), ST_OK);
    CHECK_EQ_INT(ctx.history.count, 0);

    context_destroy(&ctx);
}

int main(void) {
    printf("========================================================\n");
    printf("     RUNNING PHASE 10 FEATURE PACK F1 & F2 TESTS        \n");
    printf("========================================================\n");

    test_feature_f1_citygraph();
    test_feature_f2_history();
    test_f1_and_f2_integrated_matching();

    TEST_REPORT();

    return (g_tests_failed == 0) ? 0 : 1;
}
