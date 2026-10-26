#include "bloodgroup.h"

/*
 * bloodgroup.c - Implementation of compatibility matrix and donation eligibility logic.
 */

static const char *BG_NAMES[BG_COUNT] = {
    "A+", "A-", "B+", "B-", "AB+", "AB-", "O+", "O-"
};

/*
 * Compatibility Matrix: COMPATIBILITY_MATRIX[donor][recipient]
 * 1 indicates compatible donation, 0 indicates incompatible.
 * Row: Donor, Column: Recipient
 * Order: A+, A-, B+, B-, AB+, AB-, O+, O-
 *
 * Rules:
 *   O-  -> all 8
 *   O+  -> O+, A+, B+, AB+
 *   A-  -> A-, A+, AB-, AB+
 *   A+  -> A+, AB+
 *   B-  -> B-, B+, AB-, AB+
 *   B+  -> B+, AB+
 *   AB- -> AB-, AB+
 *   AB+ -> AB+
 */
static const int COMPATIBILITY_MATRIX[BG_COUNT][BG_COUNT] = {
    /* Donor A+   can give to: A+, AB+ */
    { 1, 0, 0, 0, 1, 0, 0, 0 },
    /* Donor A-   can give to: A+, A-, AB+, AB- */
    { 1, 1, 0, 0, 1, 1, 0, 0 },
    /* Donor B+   can give to: B+, AB+ */
    { 0, 0, 1, 0, 1, 0, 0, 0 },
    /* Donor B-   can give to: B+, B-, AB+, AB- */
    { 0, 0, 1, 1, 1, 1, 0, 0 },
    /* Donor AB+  can give to: AB+ */
    { 0, 0, 0, 0, 1, 0, 0, 0 },
    /* Donor AB-  can give to: AB+, AB- */
    { 0, 0, 0, 0, 1, 1, 0, 0 },
    /* Donor O+   can give to: A+, B+, AB+, O+ */
    { 1, 0, 1, 0, 1, 0, 1, 0 },
    /* Donor O-   can give to: ALL 8 groups */
    { 1, 1, 1, 1, 1, 1, 1, 1 }
};

/* Time: O(1), Space: O(1) */
const char *bloodgroup_to_string(BloodGroup bg) {
    if (bg >= 0 && bg < BG_COUNT) {
        return BG_NAMES[bg];
    }
    return "UNKNOWN";
}

/* Time: O(1), Space: O(1) */
BloodGroup bloodgroup_from_string(const char *str) {
    if (!str) return BG_INVALID;

    char clean[16];
    size_t len = 0;

    /* Skip leading whitespace */
    while (*str && isspace((unsigned char)*str)) {
        str++;
    }

    /* Copy uppercase without whitespace */
    while (*str && !isspace((unsigned char)*str) && len < sizeof(clean) - 1) {
        clean[len++] = (char)toupper((unsigned char)*str);
        str++;
    }
    clean[len] = '\0';

    for (int i = 0; i < BG_COUNT; i++) {
        if (strcmp(clean, BG_NAMES[i]) == 0) {
            return (BloodGroup)i;
        }
    }

    return BG_INVALID;
}

/* Time: O(1), Space: O(1) */
int bloodgroup_can_donate(BloodGroup donor, BloodGroup recipient) {
    if (donor < 0 || donor >= BG_COUNT || recipient < 0 || recipient >= BG_COUNT) {
        return 0;
    }
    return COMPATIBILITY_MATRIX[donor][recipient];
}

/* Time: O(1), Space: O(1) since BG_COUNT is a constant (8) */
int bloodgroup_get_compatible_donors(BloodGroup recipient, BloodGroup out_donors[BG_COUNT]) {
    if (recipient < 0 || recipient >= BG_COUNT || !out_donors) {
        return 0;
    }

    int count = 0;
    /* Put exact match first as primary priority */
    out_donors[count++] = recipient;

    /* Collect all other compatible donor blood groups */
    for (int d = 0; d < BG_COUNT; d++) {
        if (d != recipient && COMPATIBILITY_MATRIX[d][recipient]) {
            out_donors[count++] = (BloodGroup)d;
        }
    }

    return count;
}

/* Time: O(1), Space: O(1) */
int bloodgroup_is_eligible(int available, Date last_donation, Date today) {
    if (!available) {
        return 0; /* Manual willingness toggle is false */
    }

    /* Never donated previously -> immediately eligible */
    if (date_is_never(last_donation)) {
        return 1;
    }

    int days = date_days_between(last_donation, today);
    if (days < 0) {
        return 0; /* Future date or invalid date */
    }

    /* Exactly 90 whole days counts as eligible per Section 7 */
    return (days >= MIN_DONATION_GAP_DAYS);
}
