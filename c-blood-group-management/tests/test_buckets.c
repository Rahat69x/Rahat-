#include "common.h"
#include "date.h"
#include "bloodgroup.h"
#include "utils.h"
#include "donor.h"
#include "bst.h"
#include "hashtable.h"
#include "buckets.h"
#include "context.h"
#include "donor_service.h"
#include "testing.h"

/*
 * tests/test_buckets.c - Automated test suite for Phase 4 (Classified Buckets).
 */

static void test_buckets_basic_operations(void) {
    TEST_INIT("Classified Buckets Basic Operations");

    BloodGroupBuckets b;
    buckets_init(&b);

    for (int i = 0; i < BG_COUNT; i++) {
        CHECK_EQ_INT(buckets_get_count(&b, (BloodGroup)i), 0);
        CHECK(buckets_get_head(&b, (BloodGroup)i) == NULL);
    }

    Donor *dA1 = NULL, *dA2 = NULL, *dO = NULL;
    donor_new(&dA1, 1, "Alice A+", 25, 'F', BG_A_POS, "01700000001", "Dhaka", date_never(), 1);
    donor_new(&dA2, 2, "Arthur A+", 30, 'M', BG_A_POS, "01700000002", "Dhaka", date_never(), 1);
    donor_new(&dO, 3, "Oliver O+", 35, 'M', BG_O_POS, "01700000003", "Dhaka", date_never(), 1);

    CHECK_EQ_INT(buckets_insert(&b, dA1), ST_OK);
    CHECK_EQ_INT(buckets_insert(&b, dA2), ST_OK);
    CHECK_EQ_INT(buckets_insert(&b, dO), ST_OK);

    CHECK_EQ_INT(buckets_get_count(&b, BG_A_POS), 2);
    CHECK_EQ_INT(buckets_get_count(&b, BG_O_POS), 1);
    CHECK_EQ_INT(buckets_get_count(&b, BG_B_POS), 0);

    /* Move dA2 from A+ to B+ */
    CHECK_EQ_INT(buckets_move(&b, dA2, BG_A_POS, BG_B_POS), ST_OK);
    CHECK_EQ_INT(buckets_get_count(&b, BG_A_POS), 1);
    CHECK_EQ_INT(buckets_get_count(&b, BG_B_POS), 1);

    /* Remove dA1 */
    CHECK_EQ_INT(buckets_remove(&b, dA1), ST_OK);
    CHECK_EQ_INT(buckets_get_count(&b, BG_A_POS), 0);

    /* Free all */
    buckets_free_all(&b);
    CHECK_EQ_INT(buckets_get_count(&b, BG_O_POS), 0);
    CHECK_EQ_INT(buckets_get_count(&b, BG_B_POS), 0);

    donor_free(dA1);
    donor_free(dA2);
    donor_free(dO);
}

static void test_donor_service_with_buckets(void) {
    TEST_INIT("Donor Service Integrated with Classified Buckets");
    Date today = date_create(2026, 4, 15);

    SystemContext ctx;
    context_init(&ctx);

    int id1 = 0, id2 = 0;
    CHECK_EQ_INT(donor_service_add(&ctx, "Alice", 25, 'F', BG_AB_POS, "01711111111", "Dhaka", date_never(), 1, today, &id1), ST_OK);
    CHECK_EQ_INT(donor_service_add(&ctx, "Bob", 30, 'M', BG_AB_POS, "01722222222", "Chittagong", date_never(), 1, today, &id2), ST_OK);

    CHECK_EQ_INT(buckets_get_count(&ctx.bg_buckets, BG_AB_POS), 2);
    CHECK_EQ_INT(buckets_get_count(&ctx.bg_buckets, BG_O_POS), 0);

    /* Update Bob's blood group from AB+ to O- */
    CHECK_EQ_INT(donor_service_update(&ctx, id2, "Bob", 30, 'M', BG_O_NEG, "01722222222", "Chittagong", date_never(), 1, today), ST_OK);
    CHECK_EQ_INT(buckets_get_count(&ctx.bg_buckets, BG_AB_POS), 1);
    CHECK_EQ_INT(buckets_get_count(&ctx.bg_buckets, BG_O_NEG), 1);

    /* Delete Alice */
    CHECK_EQ_INT(donor_service_delete(&ctx, id1), ST_OK);
    CHECK_EQ_INT(buckets_get_count(&ctx.bg_buckets, BG_AB_POS), 0);
    CHECK_EQ_INT(buckets_get_count(&ctx.bg_buckets, BG_O_NEG), 1);

    context_destroy(&ctx);
    CHECK_EQ_INT(buckets_get_count(&ctx.bg_buckets, BG_O_NEG), 0);
}

int main(void) {
    printf("========================================================\n");
    printf("     RUNNING PHASE 4 BUCKETS AUTOMATED UNIT TESTS       \n");
    printf("========================================================\n");

    test_buckets_basic_operations();
    test_donor_service_with_buckets();

    TEST_REPORT();

    return (g_tests_failed == 0) ? 0 : 1;
}
