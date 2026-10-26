#ifndef DONOR_SERVICE_H
#define DONOR_SERVICE_H

#include "context.h"
#include "donor.h"
#include "bst.h"
#include "hashtable.h"

/*
 * donor_service.h - Transactional business operations on donors.
 * Layer 5 module: coordinates domain models, validation, and context structures.
 * Enforces transactional prepare/commit discipline (Blueprint v3 Rule 4, 11, 12).
 */

/*
 * Transactional Add Donor:
 *  1. Checks system capacity (MAX_DONORS -> ST_ERR_LIMIT)
 *  2. Validates all donor fields
 *  3. Verifies phone number uniqueness via Hash Table (ST_ERR_DUPLICATE) - O(1)
 *  4. Preallocates new Donor node (ST_ERR_NOMEM)
 *  5. Inserts into BST index by ID (ST_ERR_NOMEM) - O(log n)
 *  6. Inserts into Hash Table index by Phone (ST_ERR_NOMEM) - O(1)
 *  7. Commit: links to list tail, increments donor_count, increments next_donor_id.
 * If anything fails before commit, state remains 100% unchanged.
 */
Status donor_service_add(SystemContext *ctx, const char *name, int age, char gender,
                         BloodGroup bg, const char *phone, const char *city,
                         Date last_donation, int available, Date today, int *out_id);

/*
 * Transactional Update Donor:
 *  1. Finds donor by ID via BST index (ST_ERR_NOT_FOUND) - O(log n)
 *  2. Validates all new fields
 *  3. If phone changed, verifies uniqueness via Hash Table (ST_ERR_DUPLICATE) - O(1)
 *  4. Updates Hash Table index if phone changed
 *  5. Commit: mutates donor fields (BST key is immutable ID, tree structure preserved).
 */
Status donor_service_update(SystemContext *ctx, int id, const char *name, int age, char gender,
                            BloodGroup bg, const char *phone, const char *city,
                            Date last_donation, int available, Date today);

/*
 * Transactional Delete Donor:
 *  1. Finds donor by ID via BST index (ST_ERR_NOT_FOUND)
 *  2. Commit: deletes from Hash Table, deletes from BST index, unlinks from master list, frees donor memory, decrements donor_count.
 */
Status donor_service_delete(SystemContext *ctx, int id);

/*
 * Lookup donor by ID via BST Index.
 * Big-O: Time O(log n) average.
 * Returns ST_OK and sets *out_donor if found, ST_ERR_NOT_FOUND otherwise.
 */
Status donor_service_find_by_id(const SystemContext *ctx, int id, Donor **out_donor);

/*
 * Lookup donor by phone via Hash Table Index.
 * Big-O: Time O(1) average.
 * Returns ST_OK and sets *out_donor if found, ST_ERR_NOT_FOUND otherwise.
 */
Status donor_service_find_by_phone(const SystemContext *ctx, const char *phone, Donor **out_donor);

/*
 * Displays all active donors in list order (calls recursive traversal).
 */
void donor_service_display_all(const SystemContext *ctx);

/*
 * Displays all active donors in ascending order of ID (via BST in-order traversal).
 */
void donor_service_display_sorted_by_id(const SystemContext *ctx);

#endif /* DONOR_SERVICE_H */
