#include "common.h"
#include "date.h"
#include "bloodgroup.h"
#include "utils.h"
#include "donor.h"
#include "context.h"
#include "donor_service.h"
#include "request.h"
#include "queue.h"
#include "heap.h"
#include "undo.h"
#include "request_service.h"
#include "citygraph.h"
#include "history.h"
#include "storage.h"

/*
 * main.c - Blood Group Management System (Interactive Terminal Interface)
 * Complete C99 implementation featuring:
 *  - Master Singly Linked List (Donor memory owner)
 *  - Binary Search Tree (ID index with O(log n) lookups and sorted in-order traversal)
 *  - Hash Table with Separate Chaining (Phone index with O(1) duplicate checks and lookups)
 *  - 8-Group Classified Buckets (Direct O(1) group filtering)
 *  - FIFO Request Queue (Normal priority blood requests)
 *  - Max-Heap Priority Queue (Emergency blood requests: CRITICAL > URGENT)
 *  - Bounded LIFO Undo Stack (Transaction rollback for last fulfillment)
 *  - Feature F1: City Graph & Dijkstra Shortest Path (Nearest-donor road distance ranking)
 *  - Feature F2: Fulfilment History Doubly Linked List (Bounded history with bidirectional pagination)
 *  - File Persistence (Atomic write via temporary file and full index reconstruction)
 */

/* Time: O(1), Space: O(1) */
static void print_banner(void) {
    printf("\n=================================================================\n");
    printf("         RED-PULSE: BLOOD GROUP MANAGEMENT SYSTEM               \n");
    printf("        (Pure C99 Terminal Edition - All 10 Phases Active)       \n");
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
            printf("  Eligibility Status : [INELIGIBLE - Cooldown active, %d days left]\n",
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

/* Time: O(log n), Space: O(1) */
static void menu_search_by_id(const SystemContext *ctx, Date today) {
    printf("\n--- SEARCH DONOR BY ID (BST INDEX) ---\n");
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

/* Time: O(1) avg, Space: O(1) */
static void menu_search_by_phone(const SystemContext *ctx, Date today) {
    printf("\n--- SEARCH DONOR BY PHONE NUMBER (HASH TABLE INDEX) ---\n");
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

/* Time: O(log n), Space: O(1) */
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

/* Time: O(log n), Space: O(1) */
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

/* Time: O(k), Space: O(1) */
static void menu_display_by_blood_group(const SystemContext *ctx) {
    char bg_buf[16];
    int eof_flag = 0;

    printf("\n--- DISPLAY DONORS BY BLOOD GROUP (CLASSIFIED BUCKETS) ---\n");
    printf("Enter blood group to filter (A+, A-, B+, B-, AB+, AB-, O+, O-): ");
    fflush(stdout);

    Status st = utils_read_line(bg_buf, sizeof(bg_buf), &eof_flag);
    if (eof_flag) return;
    if (st != ST_OK) {
        printf("[ERROR] Input line too long.\n");
        return;
    }

    BloodGroup bg = bloodgroup_from_string(bg_buf);
    if (bg == BG_INVALID) {
        printf("[ERROR] Unrecognized blood group \"%s\".\n", bg_buf);
        return;
    }

    donor_service_display_by_blood_group(ctx, bg);
}

/* Time: O(log n) for emergency or O(1) for normal, Space: O(1) */
static void menu_submit_request(SystemContext *ctx, Date today) {
    char patient_name[MAX_NAME_LEN];
    char bg_buf[16];
    int units = 1;
    char hospital[MAX_HOSPITAL_LEN];
    char city[MAX_CITY_LEN];
    int urgency_choice = 1;
    int eof_flag = 0;

    printf("\n--- SUBMIT BLOOD REQUEST ---\n");

    /* 1. Patient Name */
    printf("Enter patient name (max %d chars, no '|'): ", MAX_NAME_LEN - 1);
    fflush(stdout);
    Status st = utils_read_text(patient_name, sizeof(patient_name), MAX_NAME_LEN - 1, &eof_flag);
    if (eof_flag) return;
    if (st != ST_OK) {
        printf("[ERROR] Invalid patient name.\n");
        return;
    }

    /* 2. Blood Group */
    printf("Enter blood group needed (A+, A-, B+, B-, AB+, AB-, O+, O-): ");
    fflush(stdout);
    st = utils_read_line(bg_buf, sizeof(bg_buf), &eof_flag);
    if (eof_flag) return;
    BloodGroup bg = bloodgroup_from_string(bg_buf);
    if (bg == BG_INVALID) {
        printf("[ERROR] Unrecognized blood group \"%s\".\n", bg_buf);
        return;
    }

    /* 3. Units Needed */
    printf("Enter units of blood needed (1 to %d): ", MAX_UNITS);
    fflush(stdout);
    st = utils_read_int(&units, 1, MAX_UNITS, &eof_flag);
    if (eof_flag) return;
    if (st != ST_OK) {
        printf("[ERROR] Units must be between 1 and %d.\n", MAX_UNITS);
        return;
    }

    /* 4. Hospital Name */
    printf("Enter hospital name (max %d chars, no '|'): ", MAX_HOSPITAL_LEN - 1);
    fflush(stdout);
    st = utils_read_text(hospital, sizeof(hospital), MAX_HOSPITAL_LEN - 1, &eof_flag);
    if (eof_flag) return;
    if (st != ST_OK) {
        printf("[ERROR] Invalid hospital name.\n");
        return;
    }

    /* 5. City */
    printf("Enter hospital city (max %d chars, no '|'): ", MAX_CITY_LEN - 1);
    fflush(stdout);
    st = utils_read_text(city, sizeof(city), MAX_CITY_LEN - 1, &eof_flag);
    if (eof_flag) return;
    if (st != ST_OK) {
        printf("[ERROR] Invalid city name.\n");
        return;
    }

    /* 6. Urgency Level */
    printf("\nSelect Request Urgency Level:\n");
    printf("  1. NORMAL   (Scheduled surgery / Non-critical -> FIFO Request Queue)\n");
    printf("  2. URGENT   (Serious condition -> Emergency Max-Heap Priority Queue)\n");
    printf("  3. CRITICAL (Life-threatening trauma -> Top Priority Emergency Max-Heap)\n");
    printf("Choice (1-3): ");
    fflush(stdout);
    st = utils_read_int(&urgency_choice, 1, 3, &eof_flag);
    if (eof_flag) return;
    if (st != ST_OK) {
        printf("[ERROR] Invalid choice. Defaulting to NORMAL.\n");
        urgency_choice = 1;
    }

    UrgencyLevel urgency = URG_NORMAL;
    if (urgency_choice == 2) urgency = URG_URGENT;
    else if (urgency_choice == 3) urgency = URG_CRITICAL;

    /* Submit via service */
    int req_id = 0;
    st = request_service_add(ctx, patient_name, bg, units, hospital, city, urgency, today, &req_id);
    if (st == ST_OK) {
        printf("\n+---------------------------------------------------------------+\n");
        printf("| [SUCCESS] Blood Request #%-4d Registered Successfully          |\n", req_id);
        printf("+---------------------------------------------------------------+\n");
        printf("  Patient Name   : %s\n", patient_name);
        printf("  Blood Group    : %s (%d unit%s)\n", bloodgroup_to_string(bg), units, units > 1 ? "s" : "");
        printf("  Location       : %s, %s\n", hospital, city);
        printf("  Urgency Level  : %s\n", urgency_to_string(urgency));
        printf("  Queue Routing  : %s\n", (urgency == URG_NORMAL) ? "Normal FIFO Queue" : "Emergency Max-Heap Priority Queue");
        printf("+---------------------------------------------------------------+\n");
    } else if (st == ST_ERR_LIMIT) {
        printf("\n[ERROR] Request capacity reached. Please process existing pending requests first.\n");
    } else {
        printf("\n[ERROR] Request submission failed: %s (code %d)\n", status_message(st), st);
    }
}

/* Time: O(n log n), Space: O(n) */
static void menu_process_next_request(SystemContext *ctx, Date today) {
    printf("\n--- PROCESS NEXT BLOOD REQUEST ---\n");
    printf("Matching by Priority: Emergency Max-Heap first, then Normal FIFO Queue.\n");
    printf("Donors ranked by: Biological Compatibility -> Dijkstra Road Distance -> Cooldown Recovery.\n");

    FulfillmentDetails details;
    Status st = request_service_process_next(ctx, today, &details);

    if (st == ST_OK) {
        printf("\n=================================================================\n");
        printf("               BLOOD REQUEST FULFILLMENT SUMMARY                 \n");
        printf("=================================================================\n");
        printf("  Request ID     : #%d\n", details.request.id);
        printf("  Patient Name   : %s\n", details.request.patient_name);
        printf("  Blood Group    : %s (%d unit%s required)\n",
               bloodgroup_to_string(details.request.blood_group),
               details.request.units, details.request.units > 1 ? "s" : "");
        printf("  Hospital/City  : %s, %s\n", details.request.hospital, details.request.city);
        printf("  Urgency Level  : %s (%s)\n",
               urgency_to_string(details.request.urgency),
               details.was_emergency ? "Dispatched from Emergency Max-Heap" : "Dispatched from Normal FIFO Queue");
        printf("-----------------------------------------------------------------\n");
        printf("MATCHED DONORS (Allocated %d of %d units):\n", details.matched_count, details.request.units);

        int src_idx = citygraph_find_city(&ctx->city_graph, details.request.city);
        int distances[MAX_CITIES];
        if (src_idx != -1) {
            citygraph_dijkstra(&ctx->city_graph, src_idx, distances);
        } else {
            for (int i = 0; i < MAX_CITIES; i++) distances[i] = INF_KM;
        }

        for (int i = 0; i < details.matched_count; i++) {
            Donor *d = details.matched_donors[i];
            int d_idx = citygraph_find_city(&ctx->city_graph, d->city);
            int dist = (src_idx != -1 && d_idx != -1) ? distances[d_idx] : INF_KM;

            printf("  [%d] Donor ID #%-4d | %-20s | Group: %-3s | City: %-12s | ",
                   i + 1, d->id, d->name, bloodgroup_to_string(d->blood_group), d->city);
            if (dist >= INF_KM) {
                printf("Distance: Road route unknown | Phone: %s\n", d->phone);
            } else {
                printf("Distance: %d km | Phone: %s\n", dist, d->phone);
            }
        }
        printf("-----------------------------------------------------------------\n");
        printf("[SUCCESS] All matched donors' cooldown dates updated to %04d-%02d-%02d.\n",
               today.year, today.month, today.day);
        printf("[AUDIT] Logged to Fulfilment History (Feature F2) & pushed to Undo Stack.\n");
        printf("=================================================================\n");
    } else if (st == ST_ERR_EMPTY) {
        printf("\n[INFO] No pending blood requests in either the Emergency Heap or Normal Queue.\n");
    } else if (st == ST_ERR_NO_DONORS) {
        printf("\n[NOTICE] Insufficient compatible and eligible donors available for the next request.\n");
        printf("Transactional Safety Guarantee: The request remains pending at the front of the queue,\n");
        printf("and zero donor records or cooldown dates were altered.\n");
    } else {
        printf("\n[ERROR] Request processing failed: %s (code %d)\n", status_message(st), st);
    }
}

/* Time: O(units), Space: O(1) */
static void menu_undo_fulfillment(SystemContext *ctx) {
    printf("\n--- UNDO LAST BLOOD REQUEST FULFILLMENT ---\n");

    BloodRequest restored;
    Status st = request_service_undo(ctx, &restored);

    if (st == ST_OK) {
        printf("\n+---------------------------------------------------------------+\n");
        printf("| [SUCCESS] Last Fulfillment Transaction Successfully Undone    |\n");
        printf("+---------------------------------------------------------------+\n");
        printf("  Restored Request ID : #%d\n", restored.id);
        printf("  Patient Name        : %s\n", restored.patient_name);
        printf("  Blood Group / Units : %s (%d unit%s)\n",
               bloodgroup_to_string(restored.blood_group), restored.units, restored.units > 1 ? "s" : "");
        printf("  Hospital / City     : %s, %s\n", restored.hospital, restored.city);
        printf("  Urgency Re-enqueued : %s (%s)\n",
               urgency_to_string(restored.urgency),
               (restored.urgency == URG_CRITICAL || restored.urgency == URG_URGENT)
                   ? "Restored to Emergency Max-Heap" : "Restored to Normal FIFO Queue");
        printf("  Donor Cooldowns     : Reverted to previous donation dates.\n");
        printf("  History Record      : Evicted from tail of Doubly Linked List.\n");
        printf("+---------------------------------------------------------------+\n");
    } else if (st == ST_ERR_EMPTY) {
        printf("\n[INFO] Undo stack is empty: No fulfillment actions to reverse.\n");
    } else {
        printf("\n[ERROR] Undo failed: %s (code %d)\n", status_message(st), st);
    }
}

/* Time: O(n), Space: O(1) */
static void menu_display_pending_requests(const SystemContext *ctx) {
    request_service_display_pending(ctx);
}

/* Time: O(page_size), Space: O(1) */
static void menu_browse_history(const SystemContext *ctx) {
    if (ctx->history.count == 0) {
        printf("\n[INFO] Fulfilment history is currently empty.\n");
        return;
    }

    const int page_size = 5;
    int page_index = 0;
    int total_pages = (ctx->history.count + page_size - 1) / page_size;
    if (total_pages == 0) total_pages = 1;

    char cmd_buf[32];
    int eof_flag = 0;

    for (;;) {
        history_display_page(&ctx->history, page_index, page_size);

        printf("\nNavigation: [N] Next Page | [P] Previous Page | [Q] Return to Main Menu\n");
        printf("Enter command (N/P/Q): ");
        fflush(stdout);

        Status st = utils_read_line(cmd_buf, sizeof(cmd_buf), &eof_flag);
        if (eof_flag) break;
        if (st != ST_OK) continue;

        utils_trim_whitespace(cmd_buf);
        char c = (char)toupper((unsigned char)cmd_buf[0]);

        if (c == 'Q' || c == '\0') {
            break;
        } else if (c == 'N') {
            if (page_index + 1 < total_pages) {
                page_index++;
            } else {
                printf("[INFO] Already on the oldest page (%d of %d).\n", page_index + 1, total_pages);
            }
        } else if (c == 'P') {
            if (page_index > 0) {
                page_index--;
            } else {
                printf("[INFO] Already on the most recent page (%d of %d).\n", page_index + 1, total_pages);
            }
        } else {
            printf("[ERROR] Unrecognized command. Please enter 'N', 'P', or 'Q'.\n");
        }
    }
}

/* Time: O(V^2 + E), Space: O(V) */
static void menu_query_road_distance(const SystemContext *ctx) {
    printf("\n--- FEATURE F1: CITY GRAPH & DIJKSTRA ROAD DISTANCE QUERY ---\n");
    printf("Registered cities in road network (%d cities):\n", ctx->city_graph.city_count);

    for (int i = 0; i < ctx->city_graph.city_count; i++) {
        printf("  %-15s%s", ctx->city_graph.names[i], ((i + 1) % 4 == 0) ? "\n" : "");
    }
    if (ctx->city_graph.city_count % 4 != 0) printf("\n");

    char city_a[MAX_CITY_LEN];
    char city_b[MAX_CITY_LEN];
    int eof_flag = 0;

    printf("\nEnter Origin City name: ");
    fflush(stdout);
    Status st = utils_read_text(city_a, sizeof(city_a), MAX_CITY_LEN - 1, &eof_flag);
    if (eof_flag || st != ST_OK) return;

    int src = citygraph_find_city(&ctx->city_graph, city_a);
    if (src == -1) {
        printf("[ERROR] City \"%s\" is not found in the road network.\n", city_a);
        return;
    }

    printf("Enter Destination City name: ");
    fflush(stdout);
    st = utils_read_text(city_b, sizeof(city_b), MAX_CITY_LEN - 1, &eof_flag);
    if (eof_flag || st != ST_OK) return;

    int dst = citygraph_find_city(&ctx->city_graph, city_b);
    if (dst == -1) {
        printf("[ERROR] City \"%s\" is not found in the road network.\n", city_b);
        return;
    }

    int dist[MAX_CITIES];
    citygraph_dijkstra(&ctx->city_graph, src, dist);

    printf("\n+---------------------------------------------------------------+\n");
    printf("| DIJKSTRA SHORTEST HIGHWAY DISTANCE                            |\n");
    printf("+---------------------------------------------------------------+\n");
    printf("  Origin City       : %s\n", ctx->city_graph.names[src]);
    printf("  Destination City  : %s\n", ctx->city_graph.names[dst]);

    if (dist[dst] >= INF_KM) {
        printf("  Road Distance     : [NO CONNECTING ROUTE / UNREACHABLE]\n");
    } else {
        printf("  Road Distance     : %d km\n", dist[dst]);
    }
    printf("+---------------------------------------------------------------+\n");
}

/* Time: O(n), Space: O(1) */
static void menu_save_to_disk(const SystemContext *ctx) {
    char path_buf[128];
    int eof_flag = 0;

    printf("\n--- SAVE SYSTEM STATE TO DISK (ATOMIC PERSISTENCE) ---\n");
    printf("Enter destination filename (press Enter for default '%s'): ", DEFAULT_DATA_FILE);
    fflush(stdout);

    Status st = utils_read_line(path_buf, sizeof(path_buf), &eof_flag);
    if (eof_flag) return;
    utils_trim_whitespace(path_buf);

    const char *filepath = (strlen(path_buf) == 0) ? DEFAULT_DATA_FILE : path_buf;

    st = storage_save(ctx, filepath);
    if (st == ST_OK) {
        printf("\n[SUCCESS] Entire system state atomically persisted to \"%s\".\n", filepath);
        printf("Records saved: %d Donors | %d Pending Requests | %d Fulfilled Records\n",
               ctx->donor_count,
               ctx->emergency_heap.size + ctx->normal_queue.count,
               ctx->history.count);
    } else {
        printf("\n[ERROR] Failed to save state to \"%s\": %s (code %d)\n",
               filepath, status_message(st), st);
    }
}

/* Time: O(n), Space: O(1) */
static void menu_load_from_disk(SystemContext *ctx) {
    char path_buf[128];
    int eof_flag = 0;

    printf("\n--- LOAD SYSTEM STATE FROM DISK ---\n");
    printf("WARNING: Loading from file will replace all current in-memory records.\n");
    printf("Enter source filename (press Enter for default '%s'): ", DEFAULT_DATA_FILE);
    fflush(stdout);

    Status st = utils_read_line(path_buf, sizeof(path_buf), &eof_flag);
    if (eof_flag) return;
    utils_trim_whitespace(path_buf);

    const char *filepath = (strlen(path_buf) == 0) ? DEFAULT_DATA_FILE : path_buf;

    int confirm = 0;
    st = utils_read_confirm("Are you sure you want to replace current in-memory state?", &confirm, &eof_flag);
    if (eof_flag || !confirm) {
        printf("[CANCELED] Load operation canceled.\n");
        return;
    }

    st = storage_load(ctx, filepath);
    if (st == ST_OK) {
        printf("\n[SUCCESS] State successfully restored from \"%s\".\n", filepath);
        printf("Reconstructed Indexes: Master List (%d), BST ID Index, Phone Hash Table,\n", ctx->donor_count);
        printf("8 Blood Buckets, Emergency Heap (%d), Normal Queue (%d), History List (%d).\n",
               ctx->emergency_heap.size, ctx->normal_queue.count, ctx->history.count);
    } else if (st == ST_ERR_NOT_FOUND) {
        printf("\n[ERROR] File \"%s\" does not exist on disk.\n", filepath);
    } else {
        printf("\n[ERROR] Failed to load data from \"%s\": %s (code %d)\n",
               filepath, status_message(st), st);
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
        int pending_emerg = ctx.emergency_heap.size;
        int pending_norm = ctx.normal_queue.count;
        int pending_total = pending_emerg + pending_norm;

        printf("System Date: %s | Donors: %d | Pending Requests: %d (Emerg: %d, Norm: %d) | Fulfilled: %d\n",
               today_str, ctx.donor_count, pending_total, pending_emerg, pending_norm, ctx.fulfilled_count);
        printf("=================================================================\n");
        printf("DONOR MANAGEMENT (Linked List, BST, Hash Table, Buckets):\n");
        printf("  1. Register New Donor\n");
        printf("  2. Search Donor by ID (BST Index)\n");
        printf("  3. Search Donor by Phone (Hash Table Index)\n");
        printf("  4. Update Donor Information\n");
        printf("  5. Delete Donor Record\n");
        printf("  6. Display All Donors (Master List Order)\n");
        printf("  7. Display All Donors (Sorted by ID - BST In-Order)\n");
        printf("  8. Display Donors by Blood Group (Classified Buckets)\n");
        printf("\nBLOOD REQUESTS & PROCESSING (Priority Heap, FIFO Queue, Dijkstra Ranking):\n");
        printf("  9. Submit Blood Request (Normal Queue or Emergency Max-Heap)\n");
        printf(" 10. Process Next Blood Request (Emergency First + Nearest-Donor Ranking)\n");
        printf(" 11. Undo Last Blood Request Fulfillment (Bounded LIFO Undo Stack)\n");
        printf(" 12. View Pending Blood Requests (Priority Heap & FIFO Queue)\n");
        printf("\nFEATURE PACK (F1 City Graph & F2 Fulfilment History):\n");
        printf(" 13. Browse Fulfilment History (Doubly Linked List with Pagination)\n");
        printf(" 14. Query City Road Distance & Shortest Path (Dijkstra)\n");
        printf("\nPERSISTENCE & UTILITIES:\n");
        printf(" 15. Save System State to Disk (Atomic File Persistence)\n");
        printf(" 16. Load System State from Disk (Reconstruct All Indexes)\n");
        printf(" 17. Query Blood Group Compatibility Matrix\n");
        printf("  0. Exit System\n");
        printf("=================================================================\n");
        printf("Enter your choice (0-17): ");
        fflush(stdout);

        Status st = utils_read_int(&choice, 0, 17, &eof_flag);
        if (eof_flag) {
            printf("\n[EOF Detected] Exiting system cleanly.\n");
            break;
        }

        if (st != ST_OK) {
            printf("[ERROR] Please enter a valid menu number between 0 and 17.\n");
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
                menu_display_by_blood_group(&ctx);
                break;
            case 9:
                menu_submit_request(&ctx, today);
                break;
            case 10:
                menu_process_next_request(&ctx, today);
                break;
            case 11:
                menu_undo_fulfillment(&ctx);
                break;
            case 12:
                menu_display_pending_requests(&ctx);
                break;
            case 13:
                menu_browse_history(&ctx);
                break;
            case 14:
                menu_query_road_distance(&ctx);
                break;
            case 15:
                menu_save_to_disk(&ctx);
                break;
            case 16:
                menu_load_from_disk(&ctx);
                break;
            case 17:
                menu_compatibility_query();
                break;
            default:
                break;
        }
    }

    context_destroy(&ctx);
    return 0;
}
