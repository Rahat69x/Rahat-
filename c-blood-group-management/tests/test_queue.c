#include "common.h"
#include "date.h"
#include "bloodgroup.h"
#include "utils.h"
#include "request.h"
#include "queue.h"
#include "testing.h"

/*
 * tests/test_queue.c - Automated test suite for Phase 5 (Request FIFO Queue).
 */

static void test_request_validation(void) {
    TEST_INIT("Blood Request Field Validation");

    CHECK_EQ_INT(request_validate_patient_name("John Doe"), 1);
    CHECK_EQ_INT(request_validate_patient_name("John|Doe"), 0); /* Pipe prohibited */
    CHECK_EQ_INT(request_validate_patient_name(""), 0);

    CHECK_EQ_INT(request_validate_blood_group(BG_O_POS), 1);
    CHECK_EQ_INT(request_validate_blood_group(BG_INVALID), 0);

    CHECK_EQ_INT(request_validate_units(1), 1);
    CHECK_EQ_INT(request_validate_units(MAX_UNITS), 1);
    CHECK_EQ_INT(request_validate_units(0), 0);
    CHECK_EQ_INT(request_validate_units(MAX_UNITS + 1), 0);

    CHECK_EQ_INT(request_validate_hospital("Dhaka Medical"), 1);
    CHECK_EQ_INT(request_validate_hospital(""), 0);

    CHECK_EQ_INT(request_validate_city("Dhaka"), 1);
    CHECK_EQ_INT(request_validate_city(""), 0);

    CHECK_EQ_INT(request_validate_urgency(URG_NORMAL), 1);
    CHECK_EQ_INT(request_validate_urgency(URG_CRITICAL), 1);
    CHECK_EQ_INT(request_validate_urgency((UrgencyLevel)99), 0);

    CHECK_EQ_INT(request_validate_fields("Jane Doe", BG_A_NEG, 2, "Square Hospital", "Dhaka", URG_NORMAL), ST_OK);
    CHECK_EQ_INT(request_validate_fields("Jane|Doe", BG_A_NEG, 2, "Square Hospital", "Dhaka", URG_NORMAL), ST_ERR_INVALID);
}

static void test_queue_fifo_operations(void) {
    TEST_INIT("Request FIFO Queue Operations");

    RequestQueue q;
    queue_init(&q);

    CHECK_EQ_INT(queue_is_empty(&q), 1);
    CHECK_EQ_INT(q.count, 0);

    BloodRequest r1, r2, r3;
    Date d = date_create(2026, 4, 15);
    request_init(&r1, 1, "Patient One", BG_A_POS, 1, "Hospital A", "Dhaka", URG_NORMAL, 1, d);
    request_init(&r2, 2, "Patient Two", BG_B_POS, 2, "Hospital B", "Chittagong", URG_NORMAL, 2, d);
    request_init(&r3, 3, "Patient Three", BG_O_POS, 1, "Hospital C", "Sylhet", URG_NORMAL, 3, d);

    /* Enqueue 3 requests */
    CHECK_EQ_INT(queue_enqueue(&q, &r1), ST_OK);
    CHECK_EQ_INT(queue_enqueue(&q, &r2), ST_OK);
    CHECK_EQ_INT(queue_enqueue(&q, &r3), ST_OK);
    CHECK_EQ_INT(q.count, 3);
    CHECK_EQ_INT(queue_is_empty(&q), 0);

    /* Peek at front */
    BloodRequest peeked;
    CHECK_EQ_INT(queue_peek(&q, &peeked), ST_OK);
    CHECK_EQ_INT(peeked.id, 1);
    CHECK_STR_EQ(peeked.patient_name, "Patient One");
    CHECK_EQ_INT(q.count, 3);

    /* Dequeue: must preserve FIFO order (r1, r2, r3) */
    BloodRequest out;
    CHECK_EQ_INT(queue_dequeue(&q, &out), ST_OK);
    CHECK_EQ_INT(out.id, 1);
    CHECK_STR_EQ(out.patient_name, "Patient One");
    CHECK_EQ_INT(q.count, 2);

    CHECK_EQ_INT(queue_dequeue(&q, &out), ST_OK);
    CHECK_EQ_INT(out.id, 2);
    CHECK_STR_EQ(out.patient_name, "Patient Two");
    CHECK_EQ_INT(q.count, 1);

    CHECK_EQ_INT(queue_dequeue(&q, &out), ST_OK);
    CHECK_EQ_INT(out.id, 3);
    CHECK_STR_EQ(out.patient_name, "Patient Three");
    CHECK_EQ_INT(q.count, 0);
    CHECK_EQ_INT(queue_is_empty(&q), 1);

    /* Dequeue on empty queue returns ST_ERR_EMPTY */
    CHECK_EQ_INT(queue_dequeue(&q, &out), ST_ERR_EMPTY);
    CHECK_EQ_INT(queue_peek(&q, &out), ST_ERR_EMPTY);

    /* Free all */
    queue_enqueue(&q, &r1);
    queue_free_all(&q);
    CHECK_EQ_INT(queue_is_empty(&q), 1);
    CHECK_EQ_INT(q.count, 0);
}

int main(void) {
    printf("========================================================\n");
    printf("     RUNNING PHASE 5 REQUEST QUEUE AUTOMATED TESTS      \n");
    printf("========================================================\n");

    test_request_validation();
    test_queue_fifo_operations();

    TEST_REPORT();

    return (g_tests_failed == 0) ? 0 : 1;
}
