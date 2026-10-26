#ifndef DONOR_SERVICE_H
#define DONOR_SERVICE_H

#include "context.h"
#include "donor.h"

/*
 * donor_service.h - Transactional business operations on donors.
 * Layer 5 module: coordinates domain models, validation, and context structures.
 * Enforces transactional prepare/commit discipline (Blueprint v3 Rule 4, 11, 12).
 */

/*
 * Transactional Add Donor:
 *  1. Checks system capacity (MAX_DONORS -> ST_ERR_LIMIT)
 *  2. Validates all donor fields
 *  3. Verifies phone number uniqueness (ST_ERR_DUPLICATE)
 *  4. Preallocates new Donor node (ST_ERR_NOMEM)
 *  5. Commit: links to list tail, increments donor_count, increments next_donor_id.
 * If anything fails before commit, state remains 100% unchanged.
 */
Status donor_service_add(SystemContext *ctx, const char *name, int age, char gender,
                         BloodGroup bg, const char *phone, const char *city,
                         Date last_donation, int available, Date today, int *out_id);

/*
 * Transactional Update Donor:
 *  1. Finds donor by ID (ST_ERR_NOT_FOUND)
 *  2. Validates all new fields
 *  3. If phone changed, verifies uniqueness (ST_ERR_DUPLICATE)
 *  4. Commit: mutates donor fields.
 */
Status donor_service_update(SystemContext *ctx, int id, const char *name, int age, char gender,
                            BloodGroup bg, const char *phone, const char *city,
                            Date last_donation, int available, Date today);

/*
 * Transactional Delete Donor:
 *  1. Finds donor by ID (ST_ERR_NOT_FOUND)
 *  2. Commit: unlinks from list, frees donor memory, decrements donor_count.
 */
Status donor_service_delete(SystemContext *ctx, int id);

/*
 * Lookup donor by ID.
 * Returns ST_OK and sets *out_donor if found, ST_ERR_NOT_FOUND otherwise.
 */
Status donor_service_find_by_id(const SystemContext *ctx, int id, Donor **out_donor);

/*
 * Lookup donor by phone.
 * Returns ST_OK and sets *out_donor if found, ST_ERR_NOT_FOUND otherwise.
 */
Status donor_service_find_by_phone(const SystemContext *ctx, const char *phone, Donor **out_donor);

/*
 * Displays all active donors in list order (calls recursive traversal).
 */
void donor_service_display_all(const SystemContext *ctx);

#endif /* DONOR_SERVICE_H */
