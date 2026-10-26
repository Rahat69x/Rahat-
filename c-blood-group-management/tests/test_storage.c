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
#include "storage.h"
#include "testing.h"

/*
 * tests/test_storage.c - Automated test suite for Phase 9 (File Persistence).
 */

static void test_atomic_save_and_load(void) {
    TEST_INIT("Atomic Save and Full Context Reconstruction");
    Date today = date_create(2026, 4, 15);
    const char *test_file = "test_bgms_storage.txt";

    SystemContext ctx1;
    context_init(&ctx1);

    /* 1. Populate donors */
    int d1 = 0, d2 = 0;
    donor_service_add(&ctx1, "Alice Walker", 25, 'F', BG_A_POS, "01711111111", "Dhaka", date_never(), 1, today, &d1);
    donor_service_add(&ctx1, "Bob Marley", 30, 'M', BG_O_NEG, "01722222222", "Chittagong", date_create(2026, 1, 1), 1, today, &d2);

    /* 2. Populate requests */
    int r1 = 0, r2 = 0;
    request_service_add(&ctx1, "Patient Normal", BG_A_POS, 1, "Hospital 1", "Dhaka", URG_NORMAL, today, &r1);
    request_service_add(&ctx1, "Patient Emergency", BG_O_NEG, 2, "Hospital 2", "Chittagong", URG_CRITICAL, today, &r2);

    /* 3. Save to disk */
    CHECK_EQ_INT(storage_save(&ctx1, test_file), ST_OK);

    /* 4. Load into a fresh context */
    SystemContext ctx2;
    context_init(&ctx2);

    CHECK_EQ_INT(storage_load(&ctx2, test_file), ST_OK);

    /* 5. Verify system counters */
    CHECK_EQ_INT(ctx2.donor_count, 2);
    CHECK_EQ_INT(ctx2.next_donor_id, ctx1.next_donor_id);
    CHECK_EQ_INT(ctx2.next_request_id, ctx1.next_request_id);

    /* 6. Verify BST index */
    Donor *found_d1 = NULL;
    CHECK_EQ_INT(donor_service_find_by_id(&ctx2, d1, &found_d1), ST_OK);
    CHECK(found_d1 != NULL);
    CHECK_STR_EQ(found_d1->name, "Alice Walker");

    /* 7. Verify Phone hash table */
    Donor *found_phone = NULL;
    CHECK_EQ_INT(donor_service_find_by_phone(&ctx2, "01722222222", &found_phone), ST_OK);
    CHECK(found_phone != NULL);
    CHECK_STR_EQ(found_phone->name, "Bob Marley");

    /* 8. Verify Classified buckets */
    CHECK_EQ_INT(buckets_get_count(&ctx2.bg_buckets, BG_A_POS), 1);
    CHECK_EQ_INT(buckets_get_count(&ctx2.bg_buckets, BG_O_NEG), 1);

    /* 9. Verify Queue and Heap */
    CHECK_EQ_INT(ctx2.normal_queue.count, 1);
    CHECK_EQ_INT(heap_size(&ctx2.emergency_heap), 1);

    BloodRequest em_top;
    CHECK_EQ_INT(heap_peek(&ctx2.emergency_heap, &em_top), ST_OK);
    CHECK_EQ_INT(em_top.urgency, URG_CRITICAL);
    CHECK_STR_EQ(em_top.patient_name, "Patient Emergency");

    /* Teardown & cleanup */
    context_destroy(&ctx1);
    context_destroy(&ctx2);
    remove(test_file);
}

static void test_storage_error_handling(void) {
    TEST_INIT("Storage Error Handling (Missing & Corrupted Files)");

    SystemContext ctx;
    context_init(&ctx);

    /* Load non-existent file */
    CHECK_EQ_INT(storage_load(&ctx, "non_existent_file_xyz_123.txt"), ST_ERR_NOT_FOUND);

    /* Load corrupted file */
    const char *corrupted_file = "test_corrupted.txt";
    FILE *f = fopen(corrupted_file, "w");
    fprintf(f, "[DONORS]\n");
    fprintf(f, "corrupted|data|with|invalid|fields\n");
    fclose(f);

    CHECK_EQ_INT(storage_load(&ctx, corrupted_file), ST_ERR_FORMAT);
    remove(corrupted_file);

    context_destroy(&ctx);
}

int main(void) {
    printf("========================================================\n");
    printf("     RUNNING PHASE 9 STORAGE AUTOMATED UNIT TESTS       \n");
    printf("========================================================\n");

    test_atomic_save_and_load();
    test_storage_error_handling();

    TEST_REPORT();

    return (g_tests_failed == 0) ? 0 : 1;
}
