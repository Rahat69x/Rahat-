#include "common.h"
#include "date.h"
#include "bloodgroup.h"
#include "utils.h"
#include "context.h"

/*
 * main.c - Phase 0 Entry Point & Minimal Demonstration Driver.
 * Provides a minimal interactive terminal menu to verify foundational modules:
 * safe line reading, integer overflow rejection, date arithmetic,
 * biological compatibility matrix, and SystemContext lifecycle.
 */

/* Time: O(1), Space: O(1) */
static void print_banner(void) {
    printf("\n=================================================================\n");
    printf("         RED-PULSE: BLOOD GROUP MANAGEMENT SYSTEM               \n");
    printf("                   (Phase 0: Setup & Foundations)                \n");
    printf("=================================================================\n");
}

/* Time: O(1), Space: O(1) */
static void demo_date_validation(void) {
    char date_buf[32];
    Date today = date_get_today();
    char today_str[16];
    date_format(today, today_str, sizeof(today_str));

    printf("\n--- DATE VALIDATION & ELIGIBILITY CHECK ---\n");
    printf("System local date today: %s\n", today_str);
    printf("Enter last donation date (YYYY-MM-DD or 'never'): ");
    fflush(stdout);

    int eof_flag = 0;
    Status st = utils_read_line(date_buf, sizeof(date_buf), &eof_flag);
    if (eof_flag) {
        printf("\n[CANCELED] EOF encountered.\n");
        return;
    }
    if (st != ST_OK) {
        printf("[ERROR] Input line too long or invalid.\n");
        return;
    }

    utils_trim_whitespace(date_buf);
    if (date_buf[0] == '\0') {
        printf("[ERROR] Date input cannot be empty.\n");
        return;
    }

    Date input_date;
    Status parse_status = date_parse_user(date_buf, &input_date, today);
    if (parse_status != ST_OK) {
        printf("[REJECTED] Invalid date \"%s\".\n", date_buf);
        printf("Rules: Must be exactly 10 characters (YYYY-MM-DD) or 'never',\n");
        printf("month 1-12, valid day in month (Gregorian leap years supported),\n");
        printf("year >= 1900, and cannot be in the future relative to today (%s).\n", today_str);
        return;
    }

    char parsed_str[16];
    date_format(input_date, parsed_str, sizeof(parsed_str));
    printf("[ACCEPTED] Successfully parsed date: %s\n", parsed_str);

    int eligible = bloodgroup_is_eligible(1, input_date, today);
    if (date_is_never(input_date)) {
        printf("Status: Donor has NEVER donated before -> ELIGIBLE to donate immediately.\n");
    } else {
        int elapsed_days = date_days_between(input_date, today);
        printf("Elapsed days since last donation: %d days.\n", elapsed_days);
        printf("Eligibility evaluation (>= %d days required): %s\n",
               MIN_DONATION_GAP_DAYS,
               eligible ? "ELIGIBLE" : "INELIGIBLE (Within 90-day cooldown period)");
    }
}

/* Time: O(1), Space: O(1) */
static void demo_compatibility_query(void) {
    char bg_buf[16];
    printf("\n--- BLOOD GROUP COMPATIBILITY MATRIX QUERY ---\n");
    printf("Enter patient blood group needed (e.g., A+, O-, B+, AB-): ");
    fflush(stdout);

    int eof_flag = 0;
    Status st = utils_read_line(bg_buf, sizeof(bg_buf), &eof_flag);
    if (eof_flag) {
        printf("\n[CANCELED] EOF encountered.\n");
        return;
    }
    if (st != ST_OK) {
        printf("[ERROR] Input line too long.\n");
        return;
    }

    BloodGroup patient_bg = bloodgroup_from_string(bg_buf);
    if (patient_bg == BG_INVALID) {
        printf("[ERROR] Unrecognized blood group \"%s\". Permitted: A+, A-, B+, B-, AB+, AB-, O+, O-.\n", bg_buf);
        return;
    }

    BloodGroup comp[BG_COUNT];
    int count = bloodgroup_get_compatible_donors(patient_bg, comp);

    printf("\nCompatible donor blood groups for Patient [%s]:\n", bloodgroup_to_string(patient_bg));
    for (int i = 0; i < count; i++) {
        printf("  %d. %-4s %s\n",
               i + 1,
               bloodgroup_to_string(comp[i]),
               (i == 0) ? "(Exact Group Match - Primary Priority)" : "(Compatible Donor Group)");
    }
}

/* Time: O(1), Space: O(1) */
static void demo_input_validation(void) {
    printf("\n--- SAFE INPUT & VALIDATORS DEMONSTRATION ---\n");
    printf("1. Integer Reading (Donor Age 18-65, rejects >9 digits to prevent overflow):\n");
    printf("Enter age: ");
    fflush(stdout);

    int age = 0;
    int eof_flag = 0;
    Status st = utils_read_int(&age, MIN_DONOR_AGE, MAX_DONOR_AGE, &eof_flag);
    if (eof_flag) {
        printf("\n[CANCELED] EOF encountered.\n");
        return;
    }
    if (st == ST_OK) {
        printf("[ACCEPTED] Valid donor age: %d\n", age);
    } else {
        printf("[REJECTED] Input invalid: must be 1-9 digits within [%d, %d].\n",
               MIN_DONOR_AGE, MAX_DONOR_AGE);
    }

    printf("\n2. Text Field Validation (ASCII 32-126, no '|' delimiter, max 50 chars):\n");
    printf("Enter donor name: ");
    fflush(stdout);

    char name_buf[64];
    st = utils_read_text(name_buf, sizeof(name_buf), MAX_NAME_LEN, &eof_flag);
    if (eof_flag) {
        printf("\n[CANCELED] EOF encountered.\n");
        return;
    }
    if (st == ST_OK) {
        printf("[ACCEPTED] Valid text: \"%s\"\n", name_buf);
    } else {
        printf("[REJECTED] Text invalid: must be printable ASCII, non-empty, trimmed, and NO '|'.\n");
    }

    printf("\n3. Phone Number Validation (10-15 digits, leading zeros kept):\n");
    printf("Enter phone number: ");
    fflush(stdout);

    char phone_buf[32];
    st = utils_read_line(phone_buf, sizeof(phone_buf), &eof_flag);
    if (eof_flag) {
        printf("\n[CANCELED] EOF encountered.\n");
        return;
    }
    if (st == ST_OK) {
        utils_trim_whitespace(phone_buf);
        if (utils_is_valid_phone(phone_buf)) {
            printf("[ACCEPTED] Valid phone: %s\n", phone_buf);
        } else {
            printf("[REJECTED] Phone \"%s\" invalid: must be 10 to 15 digits only.\n", phone_buf);
        }
    }

    printf("\n4. Yes/No Confirmation Prompt:\n");
    int confirmed = 0;
    st = utils_read_confirm("Do you wish to proceed?", &confirmed, &eof_flag);
    if (eof_flag) {
        printf("\n[CANCELED] EOF encountered.\n");
        return;
    }
    if (st == ST_OK) {
        printf("[RESULT] User answered: %s\n", confirmed ? "YES (Confirmed)" : "NO (Declined)");
    }
}

/* Time: O(1) per menu iteration, Space: O(1) */
int main(void) {
    SystemContext ctx;
    context_init(&ctx);

    int choice = -1;
    int eof_flag = 0;

    for (;;) {
        print_banner();
        printf("Context State: next_donor_id=%d | next_request_id=%d | next_sequence=%d\n",
               ctx.next_donor_id, ctx.next_request_id, ctx.next_sequence);
        printf("-----------------------------------------------------------------\n");
        printf("1. Test Date Validation & 90-Day Eligibility Check\n");
        printf("2. Test Blood Group Compatibility Matrix Query\n");
        printf("3. Test Safe Input & Field Validators\n");
        printf("0. Exit\n");
        printf("-----------------------------------------------------------------\n");
        printf("Enter choice (0-3): ");
        fflush(stdout);

        Status st = utils_read_int(&choice, 0, 3, &eof_flag);
        if (eof_flag) {
            printf("\n[EOF Detected] Exiting cleanly from main menu.\n");
            break;
        }

        if (st != ST_OK) {
            printf("[ERROR] Please enter a valid menu number (0-3).\n");
            continue;
        }

        if (choice == 0) {
            printf("\nExiting Phase 0 demonstration. Memory cleanly managed.\n");
            break;
        }

        switch (choice) {
            case 1:
                demo_date_validation();
                break;
            case 2:
                demo_compatibility_query();
                break;
            case 3:
                demo_input_validation();
                break;
            default:
                break;
        }
    }

    context_destroy(&ctx);
    return 0;
}
