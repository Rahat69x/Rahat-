#include "common.h"
#include "date.h"
#include "bloodgroup.h"
#include "utils.h"
#include "donor.h"
#include "bst.h"
#include "hashtable.h"
#include "context.h"
#include "donor_service.h"
#include "testing.h"

/*
 * tests/test_hashtable.c - Automated test suite for Phase 3 (Phone Hash Table).
 */

static void test_hash_table_basic_operations(void) {
    TEST_INIT("Hash Table Basic Operations & Separate Chaining");

    HashTable ht;
    ht_init(&ht);
    CHECK_EQ_INT(ht.count, 0);

    Donor *d1 = NULL, *d2 = NULL, *d3 = NULL;
    donor_new(&d1, 1, "Donor One", 25, 'M', BG_A_POS, "01711000001", "Dhaka", date_never(), 1);
    donor_new(&d2, 2, "Donor Two", 30, 'F', BG_B_POS, "01711000002", "Dhaka", date_never(), 1);
    donor_new(&d3, 3, "Donor Three", 35, 'M', BG_O_POS, "01711000003", "Dhaka", date_never(), 1);

    CHECK_EQ_INT(ht_insert(&ht, d1), ST_OK);
    CHECK_EQ_INT(ht_insert(&ht, d2), ST_OK);
    CHECK_EQ_INT(ht_insert(&ht, d3), ST_OK);
    CHECK_EQ_INT(ht.count, 3);

    /* Duplicate rejection */
    CHECK_EQ_INT(ht_insert(&ht, d1), ST_ERR_DUPLICATE);
    CHECK_EQ_INT(ht.count, 3);

    /* O(1) Lookups */
    CHECK(ht_search(&ht, "01711000001") == d1);
    CHECK(ht_search(&ht, "01711000002") == d2);
    CHECK(ht_search(&ht, "01711000003") == d3);
    CHECK(ht_search(&ht, "01711999999") == NULL);

    /* Deletion */
    CHECK_EQ_INT(ht_delete(&ht, "01711000002"), ST_OK);
    CHECK_EQ_INT(ht.count, 2);
    CHECK(ht_search(&ht, "01711000002") == NULL);
    CHECK(ht_search(&ht, "01711000001") == d1);
    CHECK(ht_search(&ht, "01711000003") == d3);

    /* Delete non-existent key */
    CHECK_EQ_INT(ht_delete(&ht, "01711999999"), ST_ERR_NOT_FOUND);

    /* Clean hash table */
    ht_free_all(&ht);
    CHECK_EQ_INT(ht.count, 0);
    CHECK(ht_search(&ht, "01711000001") == NULL);

    donor_free(d1);
    donor_free(d2);
    donor_free(d3);
}

static void test_donor_service_with_hashtable(void) {
    TEST_INIT("Donor Service Integrated with Phone Hash Table");
    Date today = date_create(2026, 4, 15);

    SystemContext ctx;
    context_init(&ctx);

    int id1 = 0, id2 = 0;
    CHECK_EQ_INT(donor_service_add(&ctx, "Alice", 25, 'F', BG_A_POS, "01711111111", "Dhaka", date_never(), 1, today, &id1), ST_OK);
    CHECK_EQ_INT(donor_service_add(&ctx, "Bob", 30, 'M', BG_B_POS, "01722222222", "Chittagong", date_never(), 1, today, &id2), ST_OK);

    CHECK_EQ_INT(ctx.phone_table.count, 2);

    /* O(1) phone lookup */
    Donor *found = NULL;
    CHECK_EQ_INT(donor_service_find_by_phone(&ctx, "01711111111", &found), ST_OK);
    CHECK(found != NULL);
    CHECK_STR_EQ(found->name, "Alice");

    /* Prevent duplicate phone via hash table */
    int id3 = 0;
    CHECK_EQ_INT(donor_service_add(&ctx, "Charlie", 35, 'M', BG_O_POS, "01711111111", "Sylhet", date_never(), 1, today, &id3), ST_ERR_DUPLICATE);
    CHECK_EQ_INT(ctx.donor_count, 2);

    /* Update phone number */
    CHECK_EQ_INT(donor_service_update(&ctx, id1, "Alice", 25, 'F', BG_A_POS, "01733333333", "Dhaka", date_never(), 1, today), ST_OK);
    CHECK_EQ_INT(donor_service_find_by_phone(&ctx, "01711111111", &found), ST_ERR_NOT_FOUND);
    CHECK_EQ_INT(donor_service_find_by_phone(&ctx, "01733333333", &found), ST_OK);
    CHECK_STR_EQ(found->name, "Alice");

    /* Delete donor: removes from hash table */
    CHECK_EQ_INT(donor_service_delete(&ctx, id1), ST_OK);
    CHECK_EQ_INT(donor_service_find_by_phone(&ctx, "01733333333", &found), ST_ERR_NOT_FOUND);
    CHECK_EQ_INT(ctx.phone_table.count, 1);

    context_destroy(&ctx);
    CHECK_EQ_INT(ctx.phone_table.count, 0);
    CHECK_EQ_INT(ctx.donor_count, 0);
}

int main(void) {
    printf("========================================================\n");
    printf("     RUNNING PHASE 3 HASH TABLE AUTOMATED UNIT TESTS    \n");
    printf("========================================================\n");

    test_hash_table_basic_operations();
    test_donor_service_with_hashtable();

    TEST_REPORT();

    return (g_tests_failed == 0) ? 0 : 1;
}
