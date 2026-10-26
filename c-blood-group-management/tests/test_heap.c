#include "common.h"
#include "date.h"
#include "bloodgroup.h"
#include "utils.h"
#include "request.h"
#include "heap.h"
#include "testing.h"

/*
 * tests/test_heap.c - Automated test suite for Phase 6 (Emergency Request Max-Heap).
 */

static void test_heap_priority_ordering(void) {
    TEST_INIT("Emergency Request Heap Priority Ordering");

    RequestHeap h;
    heap_init(&h);
    CHECK_EQ_INT(heap_is_empty(&h), 1);
    CHECK_EQ_INT(heap_size(&h), 0);

    Date d = date_create(2026, 4, 15);
    BloodRequest r1, r2, r3, r4, r5;
    /* r1: URGENT, seq 1 */
    request_init(&r1, 101, "Patient A", BG_A_POS, 1, "Hospital 1", "Dhaka", URG_URGENT, 1, d);
    /* r2: NORMAL, seq 2 */
    request_init(&r2, 102, "Patient B", BG_B_POS, 1, "Hospital 2", "Dhaka", URG_NORMAL, 2, d);
    /* r3: CRITICAL, seq 3 */
    request_init(&r3, 103, "Patient C", BG_O_POS, 2, "Hospital 3", "Dhaka", URG_CRITICAL, 3, d);
    /* r4: CRITICAL, seq 4 */
    request_init(&r4, 104, "Patient D", BG_AB_POS, 1, "Hospital 4", "Dhaka", URG_CRITICAL, 4, d);
    /* r5: URGENT, seq 5 */
    request_init(&r5, 105, "Patient E", BG_O_NEG, 1, "Hospital 5", "Dhaka", URG_URGENT, 5, d);

    /* Insert in arbitrary order: r2 (Normal), r1 (Urgent), r4 (Critical, seq 4), r3 (Critical, seq 3), r5 (Urgent, seq 5) */
    CHECK_EQ_INT(heap_insert(&h, &r2), ST_OK);
    CHECK_EQ_INT(heap_insert(&h, &r1), ST_OK);
    CHECK_EQ_INT(heap_insert(&h, &r4), ST_OK);
    CHECK_EQ_INT(heap_insert(&h, &r3), ST_OK);
    CHECK_EQ_INT(heap_insert(&h, &r5), ST_OK);
    CHECK_EQ_INT(heap_size(&h), 5);

    /* Peak at root: highest priority should be r3 (CRITICAL with earlier seq 3 over r4 seq 4) */
    BloodRequest out;
    CHECK_EQ_INT(heap_peek(&h, &out), ST_OK);
    CHECK_EQ_INT(out.id, 103);
    CHECK_EQ_INT(out.urgency, URG_CRITICAL);
    CHECK_EQ_INT(out.arrival_sequence, 3);

    /* 1st extraction: r3 (CRITICAL, seq 3) */
    CHECK_EQ_INT(heap_extract_max(&h, &out), ST_OK);
    CHECK_EQ_INT(out.id, 103);

    /* 2nd extraction: r4 (CRITICAL, seq 4) */
    CHECK_EQ_INT(heap_extract_max(&h, &out), ST_OK);
    CHECK_EQ_INT(out.id, 104);

    /* 3rd extraction: r1 (URGENT, seq 1 over r5 seq 5) */
    CHECK_EQ_INT(heap_extract_max(&h, &out), ST_OK);
    CHECK_EQ_INT(out.id, 101);

    /* 4th extraction: r5 (URGENT, seq 5) */
    CHECK_EQ_INT(heap_extract_max(&h, &out), ST_OK);
    CHECK_EQ_INT(out.id, 105);

    /* 5th extraction: r2 (NORMAL, seq 2) */
    CHECK_EQ_INT(heap_extract_max(&h, &out), ST_OK);
    CHECK_EQ_INT(out.id, 102);

    /* Empty extract */
    CHECK_EQ_INT(heap_size(&h), 0);
    CHECK_EQ_INT(heap_is_empty(&h), 1);
    CHECK_EQ_INT(heap_extract_max(&h, &out), ST_ERR_EMPTY);
}

int main(void) {
    printf("========================================================\n");
    printf("     RUNNING PHASE 6 EMERGENCY HEAP AUTOMATED TESTS     \n");
    printf("========================================================\n");

    test_heap_priority_ordering();

    TEST_REPORT();

    return (g_tests_failed == 0) ? 0 : 1;
}
