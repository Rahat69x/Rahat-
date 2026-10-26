#include "common.h"
#include "date.h"
#include "bloodgroup.h"
#include "utils.h"
#include "donor.h"
#include "bst.h"
#include "context.h"
#include "donor_service.h"
#include "testing.h"

/*
 * tests/test_bst.c - Automated test suite for Phase 2 (Donor BST Index).
 */

typedef struct TraversalRecord {
    int ids[32];
    int count;
} TraversalRecord;

static void record_visitor(const Donor *donor, void *user_data) {
    TraversalRecord *rec = (TraversalRecord *)user_data;
    if (donor && rec && rec->count < 32) {
        rec->ids[rec->count++] = donor->id;
    }
}

static void test_bst_basic_operations(void) {
    TEST_INIT("BST Basic Operations & In-Order Traversal");

    BSTNode *root = NULL;

    Donor *d50 = NULL, *d20 = NULL, *d70 = NULL, *d10 = NULL, *d30 = NULL, *d60 = NULL, *d80 = NULL;
    donor_new(&d50, 50, "Donor 50", 25, 'M', BG_A_POS, "0100000050", "Dhaka", date_never(), 1);
    donor_new(&d20, 20, "Donor 20", 30, 'F', BG_B_POS, "0100000020", "Dhaka", date_never(), 1);
    donor_new(&d70, 70, "Donor 70", 35, 'M', BG_O_POS, "0100000070", "Dhaka", date_never(), 1);
    donor_new(&d10, 10, "Donor 10", 22, 'F', BG_AB_POS, "0100000010", "Dhaka", date_never(), 1);
    donor_new(&d30, 30, "Donor 30", 28, 'M', BG_A_NEG, "0100000030", "Dhaka", date_never(), 1);
    donor_new(&d60, 60, "Donor 60", 40, 'M', BG_B_NEG, "0100000060", "Dhaka", date_never(), 1);
    donor_new(&d80, 80, "Donor 80", 45, 'F', BG_O_NEG, "0100000080", "Dhaka", date_never(), 1);

    /* Insert in arbitrary order */
    CHECK_EQ_INT(bst_insert(&root, d50), ST_OK);
    CHECK_EQ_INT(bst_insert(&root, d20), ST_OK);
    CHECK_EQ_INT(bst_insert(&root, d70), ST_OK);
    CHECK_EQ_INT(bst_insert(&root, d10), ST_OK);
    CHECK_EQ_INT(bst_insert(&root, d30), ST_OK);
    CHECK_EQ_INT(bst_insert(&root, d60), ST_OK);
    CHECK_EQ_INT(bst_insert(&root, d80), ST_OK);

    CHECK_EQ_INT(bst_count(root), 7);

    /* Reject duplicate key */
    CHECK_EQ_INT(bst_insert(&root, d50), ST_ERR_DUPLICATE);
    CHECK_EQ_INT(bst_count(root), 7);

    /* Search tests */
    CHECK(bst_search(root, 50) == d50);
    CHECK(bst_search(root, 20) == d20);
    CHECK(bst_search(root, 70) == d70);
    CHECK(bst_search(root, 10) == d10);
    CHECK(bst_search(root, 30) == d30);
    CHECK(bst_search(root, 60) == d60);
    CHECK(bst_search(root, 80) == d80);
    CHECK(bst_search(root, 999) == NULL);
    CHECK(bst_search(root, 0) == NULL);

    /* In-order traversal must produce strictly ascending sorted keys */
    TraversalRecord rec;
    rec.count = 0;
    bst_inorder(root, record_visitor, &rec);

    CHECK_EQ_INT(rec.count, 7);
    CHECK_EQ_INT(rec.ids[0], 10);
    CHECK_EQ_INT(rec.ids[1], 20);
    CHECK_EQ_INT(rec.ids[2], 30);
    CHECK_EQ_INT(rec.ids[3], 50);
    CHECK_EQ_INT(rec.ids[4], 60);
    CHECK_EQ_INT(rec.ids[5], 70);
    CHECK_EQ_INT(rec.ids[6], 80);

    /* Free BST only (donors cleaned separately) */
    bst_free_all(&root);
    CHECK(root == NULL);

    donor_free(d50);
    donor_free(d20);
    donor_free(d70);
    donor_free(d10);
    donor_free(d30);
    donor_free(d60);
    donor_free(d80);
}

static void test_bst_deletion_cases(void) {
    TEST_INIT("BST Deletion Cases (0, 1, and 2 children)");

    BSTNode *root = NULL;

    Donor *d50 = NULL, *d20 = NULL, *d70 = NULL, *d10 = NULL, *d30 = NULL, *d60 = NULL, *d80 = NULL;
    donor_new(&d50, 50, "D50", 25, 'M', BG_A_POS, "0100000050", "Dhaka", date_never(), 1);
    donor_new(&d20, 20, "D20", 30, 'F', BG_B_POS, "0100000020", "Dhaka", date_never(), 1);
    donor_new(&d70, 70, "D70", 35, 'M', BG_O_POS, "0100000070", "Dhaka", date_never(), 1);
    donor_new(&d10, 10, "D10", 22, 'F', BG_AB_POS, "0100000010", "Dhaka", date_never(), 1);
    donor_new(&d30, 30, "D30", 28, 'M', BG_A_NEG, "0100000030", "Dhaka", date_never(), 1);
    donor_new(&d60, 60, "D60", 40, 'M', BG_B_NEG, "0100000060", "Dhaka", date_never(), 1);
    donor_new(&d80, 80, "D80", 45, 'F', BG_O_NEG, "0100000080", "Dhaka", date_never(), 1);

    bst_insert(&root, d50);
    bst_insert(&root, d20);
    bst_insert(&root, d70);
    bst_insert(&root, d10);
    bst_insert(&root, d30);
    bst_insert(&root, d60);
    bst_insert(&root, d80);

    /* 1. Delete leaf node (key 10, 0 children) */
    CHECK_EQ_INT(bst_delete(&root, 10), ST_OK);
    CHECK(bst_search(root, 10) == NULL);
    CHECK_EQ_INT(bst_count(root), 6);

    /* 2. Delete non-existent key */
    CHECK_EQ_INT(bst_delete(&root, 999), ST_ERR_NOT_FOUND);

    /* 3. Delete node with 1 child: node 20 now has only right child (30) */
    CHECK_EQ_INT(bst_delete(&root, 20), ST_OK);
    CHECK(bst_search(root, 20) == NULL);
    CHECK(bst_search(root, 30) == d30);
    CHECK_EQ_INT(bst_count(root), 5);

    /* 4. Delete root node with 2 children (key 50) */
    CHECK_EQ_INT(bst_delete(&root, 50), ST_OK);
    CHECK(bst_search(root, 50) == NULL);
    CHECK_EQ_INT(bst_count(root), 4);

    /* Verify in-order traversal after root deletion */
    TraversalRecord rec;
    rec.count = 0;
    bst_inorder(root, record_visitor, &rec);
    CHECK_EQ_INT(rec.count, 4);
    CHECK_EQ_INT(rec.ids[0], 30);
    CHECK_EQ_INT(rec.ids[1], 60);
    CHECK_EQ_INT(rec.ids[2], 70);
    CHECK_EQ_INT(rec.ids[3], 80);

    bst_free_all(&root);
    CHECK(root == NULL);

    donor_free(d50);
    donor_free(d20);
    donor_free(d70);
    donor_free(d10);
    donor_free(d30);
    donor_free(d60);
    donor_free(d80);
}

static void test_donor_service_with_bst(void) {
    TEST_INIT("Donor Service Integrated with BST Index");
    Date today = date_create(2026, 4, 15);

    SystemContext ctx;
    context_init(&ctx);

    int id1 = 0, id2 = 0, id3 = 0;
    CHECK_EQ_INT(donor_service_add(&ctx, "Alice", 25, 'F', BG_A_POS, "0100000001", "Dhaka", date_never(), 1, today, &id1), ST_OK);
    CHECK_EQ_INT(donor_service_add(&ctx, "Bob", 30, 'M', BG_B_POS, "0100000002", "Chittagong", date_never(), 1, today, &id2), ST_OK);
    CHECK_EQ_INT(donor_service_add(&ctx, "Charlie", 35, 'M', BG_O_POS, "0100000003", "Sylhet", date_never(), 1, today, &id3), ST_OK);

    CHECK_EQ_INT(bst_count(ctx.donor_id_bst), 3);

    /* O(log n) lookup via BST */
    Donor *found = NULL;
    CHECK_EQ_INT(donor_service_find_by_id(&ctx, id2, &found), ST_OK);
    CHECK(found != NULL);
    CHECK_STR_EQ(found->name, "Bob");

    /* Delete Bob (id2) */
    CHECK_EQ_INT(donor_service_delete(&ctx, id2), ST_OK);
    CHECK_EQ_INT(bst_count(ctx.donor_id_bst), 2);
    CHECK_EQ_INT(donor_service_find_by_id(&ctx, id2, &found), ST_ERR_NOT_FOUND);

    /* Ensure remaining donors are intact */
    CHECK_EQ_INT(donor_service_find_by_id(&ctx, id1, &found), ST_OK);
    CHECK_STR_EQ(found->name, "Alice");
    CHECK_EQ_INT(donor_service_find_by_id(&ctx, id3, &found), ST_OK);
    CHECK_STR_EQ(found->name, "Charlie");

    context_destroy(&ctx);
    CHECK_EQ_INT(ctx.donor_count, 0);
    CHECK(ctx.donor_id_bst == NULL);
    CHECK(ctx.donor_list_head == NULL);
}

int main(void) {
    printf("========================================================\n");
    printf("     RUNNING PHASE 2 BST INDEX AUTOMATED UNIT TESTS     \n");
    printf("========================================================\n");

    test_bst_basic_operations();
    test_bst_deletion_cases();
    test_donor_service_with_bst();

    TEST_REPORT();

    return (g_tests_failed == 0) ? 0 : 1;
}
