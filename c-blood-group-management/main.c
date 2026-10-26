#include "common.h"
#include "date.h"
#include "bloodgroup.h"
#include "utils.h"
#include "donor.h"
#include "context.h"
#include "donor_service.h"

/*
 * main.c - Blood Group Management System (Interactive Terminal Interface)
 * Phase 1: Donor Registration, Search (by ID / Phone), Update, Deletion,
 * Master List Traversal, and Blood Compatibility Matrix Query.
 */

/* Time: O(1), Space: O(1) */
static void print_banner(void) {
    printf("\n=================================================================\n");
    printf("         RED-PULSE: BLOOD GROUP MANAGEMENT SYSTEM               \n");
    printf("                 (Phase 1: Donor Directory)                      \n");
    printf("=================================================================\n");
}

/* Time: O(1), Space: O(1) */
static void print_donor_card(const Donor *d, Date today) {
    if (!d) return;

    char date_buf[16];
    date_format(d->last_donation, date_buf, sizeof(date_buf));

    int eligible = bloodgroup_is_eligible(d->available, d->last_donation, today);

    printf("\n+---------------------------------------------------------------+\n");
    printf("| DONOR RECORD #%-5d                                            |\n", d->id);
    printf("+---------------------------------------------------------------+\n");
    printf("  Full Name          : %s\n", d->name);
    printf("  Age / Gender       : %d years old | Gender: %c\n", d->age, d->gender);
    printf("  Blood Group        : %s\n", bloodgroup_to_string(d->blood_group));
    printf("  Contact Phone      : %s\n", d->phone);
    printf("  Current City       : %s\n", d->city);
    printf("  Last Donation Date : %s\n", date_is_never(d->last_donation) ? "Never donated" : date_buf);
    printf("  Willing to Donate  : %s\n", d->available ? "YES (Active)" : "NO (Temporarily unavailable)");

    if (eligible) {
        printf("  Eligibility Status : [ELIGIBLE TO DONATE NOW]\n");
    } else {
        if (!d->available) {
            printf("  Eligibility Status : [INELIGIBLE - Donor marked unavailable]\n");
        } else {
            int days_ago = date_days_between(d->last_donation, today);
            int remaining = MIN_DONATION_GAP_DAYS - days_ago;
            printf("  Eligibility Status : [INELIGIBLE - Cooldown period active, %d days left]\n",
                   remaining > 0 ? remaining : 0);
        }
    }
    printf("+---------------------------------------------------------------+\n");
}

/* Time: O(n), Space: O(1) */
static void menu_register_donor(SystemContext *ctx, Date today) {
    char name[MAX_NAME_LEN];
    int age = 0;
    char gender_buf[16];
    char bg_buf[16];
    char phone[MAX_PHONE_LEN];
    char city[MAX_CITY_LEN];
    char date_buf[32];
    int available = 1;
    int eof_flag = 0;

    printf("\n--- REGISTER NEW BLOOD DONOR ---\n");

    /* 1. Full Name */
    printf("Enter full name (max %d chars, no '|'): ", MAX_NAME_LEN - 1);
    fflush(stdout);
    Status st = utils_read_text(name, sizeof(name), MAX_NAME_LEN - 1, &eof_flag);
    if (eof_flag) return;
    if (st != ST_OK) {
        printf("[ERROR] Invalid name. Must be 1-%d printable ASCII characters without '|'.\n", MAX_NAME_LEN - 1);
        return;
    }

    /* 2. Age */
    printf("Enter age (%d to %d): ", MIN_DONOR_AGE, MAX_DONOR_AGE);
    fflush(stdout);
    st = utils_read_int(&age, MIN_DONOR_AGE, MAX_DONOR_AGE, &eof_flag);
    if (eof_flag) return;
    if (st != ST_OK) {
        printf("[ERROR] Age must be an integer between %d and %d.\n", MIN_DONOR_AGE, MAX_DONOR_AGE);
        return;
    }

    /* 3. Gender */
    printf("Enter gender (M = Male, F = Female, O = Other): ");
    fflush(stdout);
    st = utils_read_line(gender_buf, sizeof(gender_buf), &eof_flag);
    if (eof_flag) return;
    utils_trim_whitespace(gender_buf);
    if (st != ST_OK || strlen(gender_buf) != 1 || !donor_validate_gender(gender_buf[0])) {
        printf("[ERROR] Gender must be a single character: 'M', 'F', or 'O'.\n");
        return;
    }
    char gender = gender_buf[0];

    /* 4. Blood Group */
    printf("Enter blood group (A+, A-, B+, B-, AB+, AB-, O+, O-): ");
    fflush(stdout);
    st = utils_read_line(bg_buf, sizeof(bg_buf), &eof_flag);
    if (eof_flag) return;
    BloodGroup bg = bloodgroup_from_string(bg_buf);
    if (bg == BG_INVALID) {
        printf("[ERROR] Unrecognized blood group \"%s\".\n", bg_buf);
        return;
    }

    /* 5. Contact Phone */
    printf("Enter phone number (10 to 15 digits only): ");
    fflush(stdout);
    st = utils_read_line(phone, sizeof(phone), &eof_flag);
    if (eof_flag) return;
    utils_trim_whitespace(phone);
    if (!donor_validate_phone(phone)) {
        printf("[ERROR] Phone number must consist of 10 to 15 numeric digits.\n");
        return;
    }

    /* 6. City */
    printf("Enter city (max %d chars, no '|'): ", MAX_CITY_LEN - 1);
    fflush(stdout);
    st = utils_read_text(city, sizeof(city), MAX_CITY_LEN - 1, &eof_flag);
    if (eof_flag) return;
    if (st != ST_OK) {
        printf("[ERROR] Invalid city name.\n");
        return;
    }

    /* 7. Last Donation Date */
    printf("Enter last donation date (YYYY-MM-DD or 'never'): ");
    fflush(stdout);
    st = utils_read_line(date_buf, sizeof(date_buf), &eof_flag);
    if (eof_flag) return;
    utils_trim_whitespace(date_buf);
    Date last_donation;
    if (date_parse_user(date_buf, &last_donation, today) != ST_OK) {
        printf("[ERROR] Invalid date format or date in future. Use YYYY-MM-DD or 'never'.\n");
        return;
    }

    /* 8. Available flag */
    st = utils_read_confirm("Is the donor currently available and willing to donate?", &available, &eof_flag);
    if (eof_flag) return;

    /* Execute Transactional Add */
    int new_id = 0;
    st = donor_service_add(ctx, name, age, gender, bg, phone, city, last_donation, available, today, &new_id);
    if (st == ST_OK) {
        printf("\n[SUCCESS] Donor registered successfully with Unique ID: %d\n", new_id);
        Donor *created = NULL;
        donor_service_find_by_id(ctx, new_id, &created);
        print_donor_card(created, today);
    } else if (st == ST_ERR_DUPLICATE) {
        printf("\n[ERROR] Registration rejected: A donor with phone \"%s\" already exists.\n", phone);
    } else if (st == ST_ERR_LIMIT) {
        printf("\n[ERROR] Registration rejected: Donor capacity limit (%d) reached.\n", MAX_DONORS);
    } else {
        printf("\n[ERROR] Registration failed: %s (code %d)\n", status_message(st), st);
    }
}

/* Time: O(n), Space: O(1) */
static void menu_search_by_id(const SystemContext *ctx, Date today) {
    printf("\n--- SEARCH DONOR BY ID ---\n");
    printf("Enter Donor ID to lookup: ");
    fflush(stdout);

    int id = 0;
    int eof_flag = 0;
    Status st = utils_read_int(&id, 1, 100000000, &eof_flag);
    if (eof_flag) return;
    if (st != ST_OK) {
        printf("[ERROR] Please enter a positive integer ID.\n");
        return;
    }

    Donor *d = NULL;
    st = donor_service_find_by_id(ctx, id, &d);
    if (st == ST_OK && d) {
        print_donor_card(d, today);
    } else {
        printf("[NOT FOUND] No donor found with ID #%d.\n", id);
    }
}

/* Time: O(n), Space: O(1) */
static void menu_search_by_phone(const SystemContext *ctx, Date today) {
    printf("\n--- SEARCH DONOR BY PHONE NUMBER ---\n");
    printf("Enter phone number (10 to 15 digits): ");
    fflush(stdout);

    char phone[MAX_PHONE_LEN];
    int eof_flag = 0;
    Status st = utils_read_line(phone, sizeof(phone), &eof_flag);
    if (eof_flag) return;
    utils_trim_whitespace(phone);

    if (st != ST_OK || !donor_validate_phone(phone)) {
        printf("[ERROR] Invalid phone format. Must be 10 to 15 numeric digits.\n");
        return;
    }

    Donor *d = NULL;
    st = donor_service_find_by_phone(ctx, phone, &d);
    if (st == ST_OK && d) {
        print_donor_card(d, today);
    } else {
        printf("[NOT FOUND] No donor registered with phone \"%s\".\n", phone);
    }
}

/* Time: O(n), Space: O(1) */
static void menu_update_donor(SystemContext *ctx, Date today) {
    printf("\n--- UPDATE DONOR INFORMATION ---\n");
    printf("Enter Donor ID to update: ");
    fflush(stdout);

    int id = 0;
    int eof_flag = 0;
    Status st = utils_read_int(&id, 1, 100000000, &eof_flag);
    if (eof_flag) return;
    if (st != ST_OK) {
        printf("[ERROR] Please enter a positive integer ID.\n");
        return;
    }

    Donor *d = NULL;
    st = donor_service_find_by_id(ctx, id, &d);
    if (st != ST_OK || !d) {
        printf("[NOT FOUND] No donor found with ID #%d.\n", id);
        return;
    }

    printf("\nCurrent Donor Details:\n");
    print_donor_card(d, today);

    char name[MAX_NAME_LEN];
    int age = 0;
    char gender_buf[16];
    char bg_buf[16];
    char phone[MAX_PHONE_LEN];
    char city[MAX_CITY_LEN];
    char date_buf[32];
    int available = 1;

    printf("\nEnter updated full name (max %d chars, no '|'): ", MAX_NAME_LEN - 1);
    fflush(stdout);
    st = utils_read_text(name, sizeof(name), MAX_NAME_LEN - 1, &eof_flag);
    if (eof_flag) return;
    if (st != ST_OK) {
        printf("[ERROR] Invalid name.\n");
        return;
    }

    printf("Enter updated age (%d to %d): ", MIN_DONOR_AGE, MAX_DONOR_AGE);
    fflush(stdout);
    st = utils_read_int(&age, MIN_DONOR_AGE, MAX_DONOR_AGE, &eof_flag);
    if (eof_flag) return;
    if (st != ST_OK) {
        printf("[ERROR] Invalid age.\n");
        return;
    }

    printf("Enter updated gender (M/F/O): ");
    fflush(stdout);
    st = utils_read_line(gender_buf, sizeof(gender_buf), &eof_flag);
    if (eof_flag) return;
    utils_trim_whitespace(gender_buf);
    if (st != ST_OK || strlen(gender_buf) != 1 || !donor_validate_gender(gender_buf[0])) {
        printf("[ERROR] Invalid gender.\n");
        return;
    }
    char gender = gender_buf[0];

    printf("Enter updated blood group (A+, A-, B+, B-, AB+, AB-, O+, O-): ");
    fflush(stdout);
    st = utils_read_line(bg_buf, sizeof(bg_buf), &eof_flag);
    if (eof_flag) return;
    BloodGroup bg = bloodgroup_from_string(bg_buf);
    if (bg == BG_INVALID) {
        printf("[ERROR] Unrecognized blood group.\n");
        return;
    }

    printf("Enter updated phone number (10 to 15 digits): ");
    fflush(stdout);
    st = utils_read_line(phone, sizeof(phone), &eof_flag);
    if (eof_flag) return;
    utils_trim_whitespace(phone);
    if (!donor_validate_phone(phone)) {
        printf("[ERROR] Invalid phone number.\n");
        return;
    }

    printf("Enter updated city (max %d chars, no '|'): ", MAX_CITY_LEN - 1);
    fflush(stdout);
    st = utils_read_text(city, sizeof(city), MAX_CITY_LEN - 1, &eof_flag);
    if (eof_flag) return;
    if (st != ST_OK) {
        printf("[ERROR] Invalid city name.\n");
        return;
    }

    printf("Enter updated last donation date (YYYY-MM-DD or 'never'): ");
    fflush(stdout);
    st = utils_read_line(date_buf, sizeof(date_buf), &eof_flag);
    if (eof_flag) return;
    utils_trim_whitespace(date_buf);
    Date last_donation;
    if (date_parse_user(date_buf, &last_donation, today) != ST_OK) {
        printf("[ERROR] Invalid date format or date in future.\n");
        return;
    }

    st = utils_read_confirm("Is donor currently available to donate?", &available, &eof_flag);
    if (eof_flag) return;

    /* Transactional Update Execution */
    st = donor_service_update(ctx, id, name, age, gender, bg, phone, city, last_donation, available, today);
    if (st == ST_OK) {
        printf("\n[SUCCESS] Donor #%d updated successfully.\n", id);
        print_donor_card(d, today);
    } else if (st == ST_ERR_DUPLICATE) {
        printf("\n[ERROR] Update failed: Phone number \"%s\" is already used by another donor.\n", phone);
    } else {
        printf("\n[ERROR] Update failed: %s (code %d)\n", status_message(st), st);
    }
}

/* Time: O(n), Space: O(1) */
static void menu_delete_donor(SystemContext *ctx, Date today) {
    printf("\n--- DELETE DONOR RECORD ---\n");
    printf("Enter Donor ID to delete: ");
    fflush(stdout);

    int id = 0;
    int eof_flag = 0;
    Status st = utils_read_int(&id, 1, 100000000, &eof_flag);
    if (eof_flag) return;
    if (st != ST_OK) {
        printf("[ERROR] Please enter a positive integer ID.\n");
        return;
    }

    Donor *d = NULL;
    st = donor_service_find_by_id(ctx, id, &d);
    if (st != ST_OK || !d) {
        printf("[NOT FOUND] No donor found with ID #%d.\n", id);
        return;
    }

    printf("\nDonor to be deleted:\n");
    print_donor_card(d, today);

    int confirmed = 0;
    st = utils_read_confirm("Are you sure you want to permanently delete this donor?", &confirmed, &eof_flag);
    if (eof_flag || !confirmed) {
        printf("[CANCELED] Deletion canceled. Donor #%d was NOT deleted.\n", id);
        return;
    }

    st = donor_service_delete(ctx, id);
    if (st == ST_OK) {
        printf("\n[SUCCESS] Donor #%d was permanently removed from the system.\n", id);
    } else {
        printf("\n[ERROR] Deletion failed: %s (code %d)\n", status_message(st), st);
    }
}

/* Time: O(1), Space: O(1) */
static void menu_compatibility_query(void) {
    char bg_buf[16];
    int eof_flag = 0;

    printf("\n--- BLOOD GROUP COMPATIBILITY QUERY ---\n");
    printf("Enter patient blood group needed (A+, A-, B+, B-, AB+, AB-, O+, O-): ");
    fflush(stdout);

    Status st = utils_read_line(bg_buf, sizeof(bg_buf), &eof_flag);
    if (eof_flag) return;
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

/* Time: O(1) per menu iteration, Space: O(1) */
int main(void) {
    SystemContext ctx;
    context_init(&ctx);

    Date today = date_get_today();
    char today_str[16];
    date_format(today, today_str, sizeof(today_str));

    int choice = -1;
    int eof_flag = 0;

    for (;;) {
        print_banner();
        printf("System Date: %s | Registered Donors: %d | Next ID: %d\n",
               today_str, ctx.donor_count, ctx.next_donor_id);
        printf("-----------------------------------------------------------------\n");
        printf("1. Register New Donor\n");
        printf("2. Search Donor by ID\n");
        printf("3. Search Donor by Phone\n");
        printf("4. Update Donor Information\n");
        printf("5. Delete Donor Record\n");
        printf("6. Display All Registered Donors (List Order)\n");
        printf("7. Display All Registered Donors (Sorted by ID - BST In-Order)\n");
        printf("8. Query Blood Group Compatibility Matrix\n");
        printf("0. Exit System\n");
        printf("-----------------------------------------------------------------\n");
        printf("Enter your choice (0-8): ");
        fflush(stdout);

        Status st = utils_read_int(&choice, 0, 8, &eof_flag);
        if (eof_flag) {
            printf("\n[EOF Detected] Exiting system cleanly.\n");
            break;
        }

        if (st != ST_OK) {
            printf("[ERROR] Please enter a valid menu number between 0 and 8.\n");
            continue;
        }

        if (choice == 0) {
            printf("\nExiting Red-Pulse Blood Group Management System. Goodbye!\n");
            break;
        }

        switch (choice) {
            case 1:
                menu_register_donor(&ctx, today);
                break;
            case 2:
                menu_search_by_id(&ctx, today);
                break;
            case 3:
                menu_search_by_phone(&ctx, today);
                break;
            case 4:
                menu_update_donor(&ctx, today);
                break;
            case 5:
                menu_delete_donor(&ctx, today);
                break;
            case 6:
                donor_service_display_all(&ctx);
                break;
            case 7:
                donor_service_display_sorted_by_id(&ctx);
                break;
            case 8:
                menu_compatibility_query();
                break;
            default:
                break;
        }
    }

    context_destroy(&ctx);
    return 0;
}
