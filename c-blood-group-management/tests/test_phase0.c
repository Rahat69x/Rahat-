#include "common.h"
#include "date.h"
#include "bloodgroup.h"
#include "utils.h"
#include "context.h"
#include "testing.h"

/*
 * tests/test_phase0.c - Automated unit test suite for Phase 0.
 * Verifies date math, eligibility, compatibility matrix, validators,
 * and SystemContext counters using tests/testing.h (no assert.h).
 */

static void test_date_module(void) {
    TEST_INIT("Date Module");
    Date today = date_create(2026, 4, 15);

    /* 1. Leap year rules */
    CHECK_EQ_INT(date_is_leap_year(2024), 1);
    CHECK_EQ_INT(date_is_leap_year(2026), 0);
    CHECK_EQ_INT(date_is_leap_year(1900), 0);
    CHECK_EQ_INT(date_is_leap_year(2000), 1);

    /* 2. Days in month */
    CHECK_EQ_INT(date_days_in_month(2024, 2), 29);
    CHECK_EQ_INT(date_days_in_month(2025, 2), 28);
    CHECK_EQ_INT(date_days_in_month(2026, 4), 30);
    CHECK_EQ_INT(date_days_in_month(2026, 12), 31);
    CHECK_EQ_INT(date_days_in_month(2026, 13), 0);
    CHECK_EQ_INT(date_days_in_month(2026, 0), 0);

    /* 3. Date comparison */
    Date d_early = date_create(2026, 1, 1);
    Date d_late = date_create(2026, 4, 15);
    CHECK(date_compare(d_early, d_late) < 0);
    CHECK(date_compare(d_late, d_early) > 0);
    CHECK_EQ_INT(date_compare(d_early, d_early), 0);

    /* 4. Strict date parsing for user (must use 'never', not literal sentinel) */
    Date out;
    CHECK_EQ_INT(date_parse_user("never", &out, today), ST_OK);
    CHECK(date_is_never(out));

    CHECK_EQ_INT(date_parse_user("NEVER", &out, today), ST_OK);
    CHECK(date_is_never(out));

    /* Literal sentinel rejected for user input */
    CHECK_EQ_INT(date_parse_user("0000-00-00", &out, today), ST_ERR_INVALID);

    /* Storage parser accepts literal sentinel */
    CHECK_EQ_INT(date_parse_storage("0000-00-00", &out, today), ST_OK);
    CHECK(date_is_never(out));

    /* Valid calendar date */
    CHECK_EQ_INT(date_parse_user("2026-04-15", &out, today), ST_OK);
    CHECK_EQ_INT(out.year, 2026);
    CHECK_EQ_INT(out.month, 4);
    CHECK_EQ_INT(out.day, 15);

    /* Rule 6: Future date relative to today must be rejected */
    CHECK_EQ_INT(date_parse_user("2026-04-16", &out, today), ST_ERR_INVALID);

    /* Rule 6: Date before 1900 must be rejected */
    CHECK_EQ_INT(date_parse_user("1899-12-31", &out, today), ST_ERR_INVALID);

    /* Non-leap Feb 29 must be rejected */
    CHECK_EQ_INT(date_parse_user("2025-02-29", &out, today), ST_ERR_INVALID);

    /* Leap year Feb 29 must be accepted */
    CHECK_EQ_INT(date_parse_user("2024-02-29", &out, today), ST_OK);

    /* Invalid formatting: wrong delimiter, non-10 chars, bad month */
    CHECK_EQ_INT(date_parse_user("2026/04/15", &out, today), ST_ERR_INVALID);
    CHECK_EQ_INT(date_parse_user("2026-4-15", &out, today), ST_ERR_INVALID);
    CHECK_EQ_INT(date_parse_user("2026-13-01", &out, today), ST_ERR_INVALID);
    CHECK_EQ_INT(date_parse_user("2026-04-31", &out, today), ST_ERR_INVALID);

    /* 5. Date math (integer days-from-civil) */
    Date jan1 = date_create(2026, 1, 1);
    Date apr15 = date_create(2026, 4, 15);
    int days = date_days_between(jan1, apr15);
    CHECK_EQ_INT(days, 104);

    Date feb28_leap = date_create(2024, 2, 28);
    Date mar1_leap = date_create(2024, 3, 1);
    CHECK_EQ_INT(date_days_between(feb28_leap, mar1_leap), 2);

    /* Round-trip conversion: civil -> days -> civil */
    long days_count = date_to_days_from_civil(apr15);
    Date round_trip = date_from_days_to_civil(days_count);
    CHECK_EQ_INT(date_compare(apr15, round_trip), 0);

    /* date_days_ago integer math */
    Date days_ago_10 = date_days_ago(apr15, 10);
    CHECK_EQ_INT(days_ago_10.year, 2026);
    CHECK_EQ_INT(days_ago_10.month, 4);
    CHECK_EQ_INT(days_ago_10.day, 5);
    CHECK_EQ_INT(date_days_between(days_ago_10, apr15), 10);

    /* 6. Exact 90-day cooldown boundary per Section 7 */
    Date apr1 = date_create(2026, 4, 1);
    CHECK_EQ_INT(date_days_between(jan1, apr1), 90);
    CHECK_EQ_INT(bloodgroup_is_eligible(1, jan1, apr1), 1);

    Date jan2 = date_create(2026, 1, 2);
    CHECK_EQ_INT(date_days_between(jan2, apr1), 89);
    CHECK_EQ_INT(bloodgroup_is_eligible(1, jan2, apr1), 0);

    Date dec31_2025 = date_create(2025, 12, 31);
    CHECK_EQ_INT(date_days_between(dec31_2025, apr1), 91);
    CHECK_EQ_INT(bloodgroup_is_eligible(1, dec31_2025, apr1), 1);

    /* Sentinel eligibility */
    CHECK_EQ_INT(bloodgroup_is_eligible(1, date_never(), apr1), 1);
    CHECK_EQ_INT(bloodgroup_is_eligible(0, date_never(), apr1), 0);

    /* Future date relative to today is ineligible */
    Date future_date = date_create(2026, 4, 2);
    CHECK_EQ_INT(bloodgroup_is_eligible(1, future_date, apr1), 0);
}

static void test_bloodgroup_module(void) {
    TEST_INIT("Blood Group Module");

    /* 1. Enum to string */
    CHECK_STR_EQ(bloodgroup_to_string(BG_A_POS), "A+");
    CHECK_STR_EQ(bloodgroup_to_string(BG_O_NEG), "O-");
    CHECK_STR_EQ(bloodgroup_to_string(BG_AB_NEG), "AB-");
    CHECK_STR_EQ(bloodgroup_to_string(BG_B_POS), "B+");
    CHECK_STR_EQ(bloodgroup_to_string(BG_INVALID), "UNKNOWN");

    /* 2. String to enum (case insensitivity) */
    CHECK_EQ_INT(bloodgroup_from_string("A+"), BG_A_POS);
    CHECK_EQ_INT(bloodgroup_from_string("a+"), BG_A_POS);
    CHECK_EQ_INT(bloodgroup_from_string("  o-  "), BG_O_NEG);
    CHECK_EQ_INT(bloodgroup_from_string("AB+"), BG_AB_POS);
    CHECK_EQ_INT(bloodgroup_from_string("ab-"), BG_AB_NEG);
    CHECK_EQ_INT(bloodgroup_from_string("INVALID"), BG_INVALID);
    CHECK_EQ_INT(bloodgroup_from_string(""), BG_INVALID);
    CHECK_EQ_INT(bloodgroup_from_string(NULL), BG_INVALID);

    /* 3. Biological Compatibility Matrix */
    /* O- is universal donor: can give to all 8 groups */
    for (int i = 0; i < BG_COUNT; i++) {
        CHECK_EQ_INT(bloodgroup_can_donate(BG_O_NEG, (BloodGroup)i), 1);
    }

    /* AB+ is universal recipient: can receive from all 8 groups */
    for (int i = 0; i < BG_COUNT; i++) {
        CHECK_EQ_INT(bloodgroup_can_donate((BloodGroup)i, BG_AB_POS), 1);
    }

    /* A+ donor can give to A+ and AB+ only */
    CHECK_EQ_INT(bloodgroup_can_donate(BG_A_POS, BG_A_POS), 1);
    CHECK_EQ_INT(bloodgroup_can_donate(BG_A_POS, BG_AB_POS), 1);
    CHECK_EQ_INT(bloodgroup_can_donate(BG_A_POS, BG_B_POS), 0);
    CHECK_EQ_INT(bloodgroup_can_donate(BG_A_POS, BG_O_POS), 0);
    CHECK_EQ_INT(bloodgroup_can_donate(BG_A_POS, BG_A_NEG), 0);

    /* B+ donor can give to B+ and AB+ only */
    CHECK_EQ_INT(bloodgroup_can_donate(BG_B_POS, BG_B_POS), 1);
    CHECK_EQ_INT(bloodgroup_can_donate(BG_B_POS, BG_AB_POS), 1);
    CHECK_EQ_INT(bloodgroup_can_donate(BG_B_POS, BG_A_POS), 0);

    /* O+ donor can give to O+, A+, B+, AB+ */
    CHECK_EQ_INT(bloodgroup_can_donate(BG_O_POS, BG_O_POS), 1);
    CHECK_EQ_INT(bloodgroup_can_donate(BG_O_POS, BG_A_POS), 1);
    CHECK_EQ_INT(bloodgroup_can_donate(BG_O_POS, BG_B_POS), 1);
    CHECK_EQ_INT(bloodgroup_can_donate(BG_O_POS, BG_AB_POS), 1);
    CHECK_EQ_INT(bloodgroup_can_donate(BG_O_POS, BG_O_NEG), 0);

    /* 4. Priority query list */
    BloodGroup comp[BG_COUNT];
    int count = bloodgroup_get_compatible_donors(BG_A_POS, comp);
    CHECK_EQ_INT(count, 4);
    CHECK_EQ_INT(comp[0], BG_A_POS); /* Exact match at index 0 */
}

static void test_utils_module(void) {
    TEST_INIT("Utils Module");

    /* 1. Phone validation (10-15 numeric digits) */
    CHECK_EQ_INT(utils_is_valid_phone("01711000001"), 1);
    CHECK_EQ_INT(utils_is_valid_phone("1234567890"), 1);
    CHECK_EQ_INT(utils_is_valid_phone("123456789012345"), 1);
    CHECK_EQ_INT(utils_is_valid_phone("123456789"), 0);         /* 9 digits: too short */
    CHECK_EQ_INT(utils_is_valid_phone("1234567890123456"), 0);   /* 16 digits: too long */
    CHECK_EQ_INT(utils_is_valid_phone("01711-00000"), 0);       /* Contains dash */
    CHECK_EQ_INT(utils_is_valid_phone("01711a00000"), 0);       /* Contains letter */
    CHECK_EQ_INT(utils_is_valid_phone(NULL), 0);

    /* 2. Age validation (18-65 inclusive) */
    CHECK_EQ_INT(utils_is_valid_age(18), 1);
    CHECK_EQ_INT(utils_is_valid_age(65), 1);
    CHECK_EQ_INT(utils_is_valid_age(30), 1);
    CHECK_EQ_INT(utils_is_valid_age(17), 0);
    CHECK_EQ_INT(utils_is_valid_age(66), 0);
    CHECK_EQ_INT(utils_is_valid_age(0), 0);

    /* 3. Text field validation: ASCII 32-126, trimmed, non-empty, NO '|' */
    CHECK_EQ_INT(utils_is_valid_text("John Doe", 50), 1);
    CHECK_EQ_INT(utils_is_valid_text("City Hospital", 50), 1);
    CHECK_EQ_INT(utils_is_valid_text("Dhaka-1200", 30), 1);

    /* Forbidden '|' delimiter */
    CHECK_EQ_INT(utils_is_valid_text("John|Doe", 50), 0);

    /* Untrimmed strings must be rejected by validator */
    CHECK_EQ_INT(utils_is_valid_text(" John Doe", 50), 0);
    CHECK_EQ_INT(utils_is_valid_text("John Doe ", 50), 0);

    /* Empty string and NULL */
    CHECK_EQ_INT(utils_is_valid_text("", 50), 0);
    CHECK_EQ_INT(utils_is_valid_text(NULL, 50), 0);

    /* Exceeds maximum length */
    CHECK_EQ_INT(utils_is_valid_text("123456", 5), 0);
    CHECK_EQ_INT(utils_is_valid_text("12345", 5), 1);

    /* 4. Whitespace trimming */
    char buf[64] = "  hello world   ";
    utils_trim_whitespace(buf);
    CHECK_STR_EQ(buf, "hello world");

    char empty_buf[32] = "     ";
    utils_trim_whitespace(empty_buf);
    CHECK_STR_EQ(empty_buf, "");

    /* 5. Safe bounded string copy */
    char dest[8];
    utils_copy_string(dest, "123456789", sizeof(dest));
    CHECK_EQ_INT(strlen(dest), 7);
    CHECK_EQ_INT(dest[7], '\0');
    CHECK_STR_EQ(dest, "1234567");

    /* 6. Integer parsing and overflow prevention (Hard Constraint 2: reject >9 digits) */
    int val = 0;
    CHECK_EQ_INT(utils_parse_int("123456789", &val, 0, 999999999), ST_OK);
    CHECK_EQ_INT(val, 123456789);

    /* 10 digits must be rejected before strtol */
    CHECK_EQ_INT(utils_parse_int("1000000000", &val, 0, 1000000000), ST_ERR_INVALID);
    CHECK_EQ_INT(utils_parse_int("9999999999", &val, 0, 1000000000), ST_ERR_INVALID);

    /* Non-numeric and whitespace */
    CHECK_EQ_INT(utils_parse_int("  42  ", &val, 0, 100), ST_OK);
    CHECK_EQ_INT(val, 42);
    CHECK_EQ_INT(utils_parse_int("42abc", &val, 0, 100), ST_ERR_INVALID);
    CHECK_EQ_INT(utils_parse_int("abc", &val, 0, 100), ST_ERR_INVALID);
    CHECK_EQ_INT(utils_parse_int("", &val, 0, 100), ST_ERR_INVALID);

    /* Range bounds */
    CHECK_EQ_INT(utils_parse_int("17", &val, 18, 65), ST_ERR_INVALID);
    CHECK_EQ_INT(utils_parse_int("66", &val, 18, 65), ST_ERR_INVALID);
    CHECK_EQ_INT(utils_parse_int("18", &val, 18, 65), ST_OK);
    CHECK_EQ_INT(utils_parse_int("65", &val, 18, 65), ST_OK);
}

static void test_context_module(void) {
    TEST_INIT("Context Module");
    SystemContext ctx;
    context_init(&ctx);

    CHECK_EQ_INT(ctx.next_donor_id, 1);
    CHECK_EQ_INT(ctx.next_request_id, 1);
    CHECK_EQ_INT(ctx.next_sequence, 1);
    CHECK_EQ_INT(ctx.fulfilled_count, 0);

    context_destroy(&ctx);
    CHECK_EQ_INT(ctx.next_donor_id, 1);
}

int main(void) {
    printf("========================================================\n");
    printf("     RUNNING PHASE 0 AUTOMATED UNIT TEST SUITE          \n");
    printf("========================================================\n");

    test_date_module();
    test_bloodgroup_module();
    test_utils_module();
    test_context_module();

    TEST_REPORT();

    return (g_tests_failed == 0) ? 0 : 1;
}
