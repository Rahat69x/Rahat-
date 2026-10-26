#include "common.h"
#include "date.h"
#include "bloodgroup.h"
#include "utils.h"
#include "donor.h"
#include "context.h"
#include "donor_service.h"
#include "testing.h"

/*
 * tests/test_donor.c - Automated test suite for Phase 1 (Donor List & Validation).
 * Uses tests/testing.h (no assert.h).
 */

static void test_field_validators(void) {
    TEST_INIT("Donor Field Validators");
    Date today = date_create(2026, 4, 15);

    /* 1. Name validation (ASCII 32-126, no '|', max 49 chars) */
    CHECK_EQ_INT(donor_validate_name("Alice Rahman"), 1);
    CHECK_EQ_INT(donor_validate_name("Bob|Chowdhury"), 0);     /* Forbidden pipe */
    CHECK_EQ_INT(donor_validate_name(""), 0);                  /* Empty */
    CHECK_EQ_INT(donor_validate_name(NULL), 0);
    CHECK_EQ_INT(donor_validate_name(" Alice"), 0);            /* Untrimmed leading */
    CHECK_EQ_INT(donor_validate_name("Alice "), 0);            /* Untrimmed trailing */

    /* 2. Age validation (18-65 inclusive) */
    CHECK_EQ_INT(donor_validate_age(18), 1);
    CHECK_EQ_INT(donor_validate_age(65), 1);
    CHECK_EQ_INT(donor_validate_age(30), 1);
    CHECK_EQ_INT(donor_validate_age(17), 0);
    CHECK_EQ_INT(donor_validate_age(66), 0);

    /* 3. Gender validation ('M', 'F', 'O', case-insensitive) */
    CHECK_EQ_INT(donor_validate_gender('M'), 1);
    CHECK_EQ_INT(donor_validate_gender('m'), 1);
    CHECK_EQ_INT(donor_validate_gender('F'), 1);
    CHECK_EQ_INT(donor_validate_gender('f'), 1);
    CHECK_EQ_INT(donor_validate_gender('O'), 1);
    CHECK_EQ_INT(donor_validate_gender('o'), 1);
    CHECK_EQ_INT(donor_validate_gender('X'), 0);
    CHECK_EQ_INT(donor_validate_gender('?'), 0);

    /* 4. Blood group validation (0..7) */
    CHECK_EQ_INT(donor_validate_blood_group(BG_A_POS), 1);
    CHECK_EQ_INT(donor_validate_blood_group(BG_O_NEG), 1);
    CHECK_EQ_INT(donor_validate_blood_group(BG_COUNT), 0);
    CHECK_EQ_INT(donor_validate_blood_group(BG_INVALID), 0);

    /* 5. Phone validation (10-15 digits only) */
    CHECK_EQ_INT(donor_validate_phone("01711223344"), 1);
    CHECK_EQ_INT(donor_validate_phone("1234567890"), 1);
    CHECK_EQ_INT(donor_validate_phone("123456789012345"), 1);
    CHECK_EQ_INT(donor_validate_phone("123456789"), 0);         /* 9 digits */
    CHECK_EQ_INT(donor_validate_phone("1234567890123456"), 0);   /* 16 digits */
    CHECK_EQ_INT(donor_validate_phone("01711-22334"), 0);       /* Dash prohibited */

    /* 6. City validation (ASCII 32-126, no '|', max 29 chars) */
    CHECK_EQ_INT(donor_validate_city("Dhaka"), 1);
    CHECK_EQ_INT(donor_validate_city("Chittagong-Port"), 1);
    CHECK_EQ_INT(donor_validate_city("Dhaka|North"), 0);        /* Pipe prohibited */
    CHECK_EQ_INT(donor_validate_city(""), 0);

    /* 7. Last donation date validation */
    CHECK_EQ_INT(donor_validate_last_donation(date_never(), today), 1);
    CHECK_EQ_INT(donor_validate_last_donation(date_create(2026, 1, 1), today), 1);
    CHECK_EQ_INT(donor_validate_last_donation(date_create(2026, 4, 15), today), 1);
    CHECK_EQ_INT(donor_validate_last_donation(date_create(2026, 4, 16), today), 0); /* Future */
    CHECK_EQ_INT(donor_validate_last_donation(date_create(1899, 12, 31), today), 0); /* < 1900 */
    CHECK_EQ_INT(donor_validate_last_donation(date_create(2025, 2, 29), today), 0); /* Non-leap */

    /* 8. Available flag (0 or 1) */
    CHECK_EQ_INT(donor_validate_available(1), 1);
    CHECK_EQ_INT(donor_validate_available(0), 1);
    CHECK_EQ_INT(donor_validate_available(2), 0);
    CHECK_EQ_INT(donor_validate_available(-1), 0);

    /* 9. Combined validator */
    CHECK_EQ_INT(donor_validate_fields("Rahat Khan", 25, 'M', BG_O_POS,
                                       "01711000001", "Dhaka",
                                       date_create(2026, 1, 1), 1, today), ST_OK);

    CHECK_EQ_INT(donor_validate_fields("Rahat|Khan", 25, 'M', BG_O_POS,
                                       "01711000001", "Dhaka",
                                       date_create(2026, 1, 1), 1, today), ST_ERR_INVALID);
}

static void test_donor_linked_list_operations(void) {
    TEST_INIT("Donor Linked List Operations");

    Donor *head = NULL;
    Donor *tail = NULL;

    Donor *d1 = NULL;
    Donor *d2 = NULL;
    Donor *d3 = NULL;

    CHECK_EQ_INT(donor_new(&d1, 1, "Alice", 25, 'F', BG_A_POS, "0100000001", "Dhaka", date_never(), 1), ST_OK);
    CHECK_EQ_INT(donor_new(&d2, 2, "Bob", 30, 'M', BG_B_POS, "0100000002", "Sylhet", date_never(), 1), ST_OK);
    CHECK_EQ_INT(donor_new(&d3, 3, "Charlie", 35, 'M', BG_O_POS, "0100000003", "Khulna", date_never(), 1), ST_OK);

    /* Append to empty list */
    donor_list_append(&head, &tail, d1);
    CHECK(head == d1);
    CHECK(tail == d1);
    CHECK(head->next == NULL);

    /* Append second node */
    donor_list_append(&head, &tail, d2);
    CHECK(head == d1);
    CHECK(tail == d2);
    CHECK(d1->next == d2);
    CHECK(d2->next == NULL);

    /* Append third node */
    donor_list_append(&head, &tail, d3);
    CHECK(head == d1);
    CHECK(tail == d3);
    CHECK(d2->next == d3);

    /* Search by ID and Phone */
    CHECK(donor_list_find_by_id(head, 2) == d2);
    CHECK(donor_list_find_by_id(head, 99) == NULL);
    CHECK(donor_list_find_by_phone(head, "0100000003") == d3);
    CHECK(donor_list_find_by_phone(head, "0999999999") == NULL);

    /* Unlink middle node (d2) */
    donor_list_unlink(&head, &tail, d2);
    CHECK(head == d1);
    CHECK(tail == d3);
    CHECK(d1->next == d3);
    CHECK(d2->next == NULL);
    donor_free(d2);

    /* Unlink head node (d1) */
    donor_list_unlink(&head, &tail, d1);
    CHECK(head == d3);
    CHECK(tail == d3);
    CHECK(d1->next == NULL);
    donor_free(d1);

    /* Unlink remaining single node (d3) */
    donor_list_unlink(&head, &tail, d3);
    CHECK(head == NULL);
    CHECK(tail == NULL);
    donor_free(d3);

    /* Append and free all */
    Donor *d4 = NULL;
    Donor *d5 = NULL;
    donor_new(&d4, 4, "David", 40, 'M', BG_AB_POS, "0100000004", "Rajshahi", date_never(), 1);
    donor_new(&d5, 5, "Emma", 22, 'F', BG_O_NEG, "0100000005", "Barisal", date_never(), 1);
    donor_list_append(&head, &tail, d4);
    donor_list_append(&head, &tail, d5);
    CHECK(head != NULL && tail != NULL);

    donor_list_free_all(&head, &tail);
    CHECK(head == NULL);
    CHECK(tail == NULL);
}

static void test_donor_service_transactional(void) {
    TEST_INIT("Donor Service Transactional CRUD");
    Date today = date_create(2026, 4, 15);

    SystemContext ctx;
    context_init(&ctx);

    CHECK_EQ_INT(ctx.donor_count, 0);
    CHECK_EQ_INT(ctx.next_donor_id, 1);

    /* 1. Add Donor 1 */
    int id1 = 0;
    Status st1 = donor_service_add(&ctx, "Alice Rahman", 24, 'F', BG_A_POS,
                                   "0100000001", "Dhaka",
                                   date_create(2026, 1, 1), 1, today, &id1);
    CHECK_EQ_INT(st1, ST_OK);
    CHECK_EQ_INT(id1, 1);
    CHECK_EQ_INT(ctx.donor_count, 1);
    CHECK_EQ_INT(ctx.next_donor_id, 2);

    /* 2. Add Donor 2 */
    int id2 = 0;
    Status st2 = donor_service_add(&ctx, "Bob Chowdhury", 32, 'M', BG_O_POS,
                                   "0100000002", "Chittagong",
                                   date_never(), 1, today, &id2);
    CHECK_EQ_INT(st2, ST_OK);
    CHECK_EQ_INT(id2, 2);
    CHECK_EQ_INT(ctx.donor_count, 2);
    CHECK_EQ_INT(ctx.next_donor_id, 3);

    /* 3. Reject duplicate phone (Transaction Rollback: state must not change) */
    int id_dup = 0;
    Status st_dup = donor_service_add(&ctx, "Charlie Hasan", 28, 'M', BG_B_POS,
                                      "0100000001", /* Duplicate phone of Alice */
                                      "Sylhet", date_never(), 1, today, &id_dup);
    CHECK_EQ_INT(st_dup, ST_ERR_DUPLICATE);
    CHECK_EQ_INT(ctx.donor_count, 2);      /* Count untouched */
    CHECK_EQ_INT(ctx.next_donor_id, 3);    /* ID counter not consumed */

    /* 4. Reject invalid field (Underage 17) */
    int id_inv = 0;
    Status st_inv = donor_service_add(&ctx, "Diana Prince", 17, 'F', BG_AB_POS,
                                      "0100000003", "Khulna",
                                      date_never(), 1, today, &id_inv);
    CHECK_EQ_INT(st_inv, ST_ERR_INVALID);
    CHECK_EQ_INT(ctx.donor_count, 2);
    CHECK_EQ_INT(ctx.next_donor_id, 3);

    /* 5. Reject pipe in name */
    Status st_pipe = donor_service_add(&ctx, "Diana|Prince", 22, 'F', BG_AB_POS,
                                       "0100000003", "Khulna",
                                       date_never(), 1, today, &id_inv);
    CHECK_EQ_INT(st_pipe, ST_ERR_INVALID);
    CHECK_EQ_INT(ctx.donor_count, 2);
    CHECK_EQ_INT(ctx.next_donor_id, 3);

    /* 6. Lookup by ID and Phone */
    Donor *found = NULL;
    CHECK_EQ_INT(donor_service_find_by_id(&ctx, 1, &found), ST_OK);
    CHECK(found != NULL);
    CHECK_STR_EQ(found->name, "Alice Rahman");

    CHECK_EQ_INT(donor_service_find_by_id(&ctx, 999, &found), ST_ERR_NOT_FOUND);

    CHECK_EQ_INT(donor_service_find_by_phone(&ctx, "0100000002", &found), ST_OK);
    CHECK(found != NULL);
    CHECK_STR_EQ(found->name, "Bob Chowdhury");

    /* 7. Update Donor 1 (Change city and availability) */
    Status st_up = donor_service_update(&ctx, 1, "Alice Rahman", 25, 'F', BG_A_POS,
                                        "0100000001", "Dhaka-North",
                                        date_create(2026, 1, 1), 0, today);
    CHECK_EQ_INT(st_up, ST_OK);
    CHECK_EQ_INT(donor_service_find_by_id(&ctx, 1, &found), ST_OK);
    CHECK_EQ_INT(found->age, 25);
    CHECK_STR_EQ(found->city, "Dhaka-North");
    CHECK_EQ_INT(found->available, 0);

    /* 8. Update with phone of another donor rejected */
    Status st_up_dup = donor_service_update(&ctx, 1, "Alice Rahman", 25, 'F', BG_A_POS,
                                            "0100000002", /* Belongs to Bob */
                                            "Dhaka-North", date_create(2026, 1, 1), 0, today);
    CHECK_EQ_INT(st_up_dup, ST_ERR_DUPLICATE);
    CHECK_STR_EQ(found->phone, "0100000001"); /* Original phone preserved */

    /* 9. Delete Donor 1 */
    CHECK_EQ_INT(donor_service_delete(&ctx, 1), ST_OK);
    CHECK_EQ_INT(ctx.donor_count, 1);
    CHECK_EQ_INT(donor_service_find_by_id(&ctx, 1, &found), ST_ERR_NOT_FOUND);

    /* 10. Delete non-existent donor */
    CHECK_EQ_INT(donor_service_delete(&ctx, 999), ST_ERR_NOT_FOUND);

    /* 11. Clean context teardown */
    context_destroy(&ctx);
    CHECK_EQ_INT(ctx.donor_count, 0);
    CHECK(ctx.donor_list_head == NULL);
    CHECK(ctx.donor_list_tail == NULL);
}

int main(void) {
    printf("========================================================\n");
    printf("     RUNNING PHASE 1 AUTOMATED UNIT TEST SUITE          \n");
    printf("========================================================\n");

    test_field_validators();
    test_donor_linked_list_operations();
    test_donor_service_transactional();

    TEST_REPORT();

    return (g_tests_failed == 0) ? 0 : 1;
}
