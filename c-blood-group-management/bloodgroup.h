#ifndef BLOODGROUP_H
#define BLOODGROUP_H

#include "common.h"
#include "date.h"

/*
 * bloodgroup.h - Enumeration, string conversions, biological compatibility
 * matrix, and donation eligibility checks.
 * Layer 2 module: depends only on common.h and date.h.
 */

typedef enum {
    BG_A_POS = 0,   /* A+  */
    BG_A_NEG,       /* A-  */
    BG_B_POS,       /* B+  */
    BG_B_NEG,       /* B-  */
    BG_AB_POS,      /* AB+ */
    BG_AB_NEG,      /* AB- */
    BG_O_POS,       /* O+  */
    BG_O_NEG,       /* O-  */
    BG_COUNT,       /* 8 standard ABO/Rh groups */
    BG_INVALID = -1
} BloodGroup;

/* Convert BloodGroup enum to string representation (e.g. "A+", "O-") */
const char *bloodgroup_to_string(BloodGroup bg);

/* Convert string representation (case-insensitive) to BloodGroup enum */
BloodGroup bloodgroup_from_string(const char *str);

/* Checks biological compatibility: returns 1 if donor can give to recipient, 0 otherwise */
int bloodgroup_can_donate(BloodGroup donor, BloodGroup recipient);

/*
 * Populates out_donors array with all donor groups compatible with the recipient.
 * Places exact match at index 0.
 * Returns the count of compatible groups.
 */
int bloodgroup_get_compatible_donors(BloodGroup recipient, BloodGroup out_donors[BG_COUNT]);

/*
 * Evaluates donor eligibility according to Section 7:
 *  - available flag must be true (non-zero)
 *  - AND (never donated OR at least 90 whole days have passed since last donation)
 * Exactly 90 days qualifies as eligible.
 */
int bloodgroup_is_eligible(int available, Date last_donation, Date today);

#endif /* BLOODGROUP_H */
