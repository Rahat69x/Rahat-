#include "donor_service.h"

/*
 * donor_service.c - Transactional donor operations implementation.
 */

/* Time: O(n), Space: O(1) where n is donor count (phone scan in Phase 1) */
Status donor_service_add(SystemContext *ctx, const char *name, int age, char gender,
                         BloodGroup bg, const char *phone, const char *city,
                         Date last_donation, int available, Date today, int *out_id) {
    if (!ctx) return ST_ERR_INVALID;

    /* 1. Capacity limit check */
    if (ctx->donor_count >= MAX_DONORS) {
        return ST_ERR_LIMIT;
    }

    /* 2. Field validation */
    Status st = donor_validate_fields(name, age, gender, bg, phone, city,
                                      last_donation, available, today);
    if (st != ST_OK) {
        return st;
    }

    /* 3. Phone uniqueness check (linear scan in Phase 1; hash in Phase 3) */
    if (donor_list_find_by_phone(ctx->donor_list_head, phone)) {
        return ST_ERR_DUPLICATE;
    }

    /* 4. Prepare phase: allocate node up front */
    Donor *new_donor = NULL;
    st = donor_new(&new_donor, ctx->next_donor_id, name, age, gender,
                   bg, phone, city, last_donation, available);
    if (st != ST_OK) {
        return st;
    }

    /* 5. Commit phase: link into master list and update counters */
    donor_list_append(&ctx->donor_list_head, &ctx->donor_list_tail, new_donor);
    if (out_id) {
        *out_id = ctx->next_donor_id;
    }
    ctx->next_donor_id++;
    ctx->donor_count++;

    return ST_OK;
}

/* Time: O(n), Space: O(1) where n is donor count */
Status donor_service_update(SystemContext *ctx, int id, const char *name, int age, char gender,
                            BloodGroup bg, const char *phone, const char *city,
                            Date last_donation, int available, Date today) {
    if (!ctx || id <= 0) return ST_ERR_INVALID;

    /* 1. Find target donor */
    Donor *d = donor_list_find_by_id(ctx->donor_list_head, id);
    if (!d) {
        return ST_ERR_NOT_FOUND;
    }

    /* 2. Validate all new fields */
    Status st = donor_validate_fields(name, age, gender, bg, phone, city,
                                      last_donation, available, today);
    if (st != ST_OK) {
        return st;
    }

    /* 3. If phone changed, verify uniqueness against other donors */
    if (strcmp(d->phone, phone) != 0) {
        Donor *existing = donor_list_find_by_phone(ctx->donor_list_head, phone);
        if (existing) {
            return ST_ERR_DUPLICATE;
        }
    }

    /* 4. Commit phase: mutate fields (in Phase 1; index relinking added in later phases) */
    utils_copy_string(d->name, name, sizeof(d->name));
    d->age = age;
    d->gender = (char)toupper((unsigned char)gender);
    d->blood_group = bg;
    utils_copy_string(d->phone, phone, sizeof(d->phone));
    utils_copy_string(d->city, city, sizeof(d->city));
    d->last_donation = last_donation;
    d->available = available;

    return ST_OK;
}

/* Time: O(n), Space: O(1) where n is donor count */
Status donor_service_delete(SystemContext *ctx, int id) {
    if (!ctx || id <= 0) return ST_ERR_INVALID;

    Donor *d = donor_list_find_by_id(ctx->donor_list_head, id);
    if (!d) {
        return ST_ERR_NOT_FOUND;
    }

    /* Commit phase: unlink from list, free memory, update counter */
    donor_list_unlink(&ctx->donor_list_head, &ctx->donor_list_tail, d);
    donor_free(d);
    ctx->donor_count--;

    return ST_OK;
}

/* Time: O(n), Space: O(1) where n is donor count */
Status donor_service_find_by_id(const SystemContext *ctx, int id, Donor **out_donor) {
    if (!ctx || id <= 0 || !out_donor) return ST_ERR_INVALID;
    *out_donor = NULL;

    Donor *d = donor_list_find_by_id(ctx->donor_list_head, id);
    if (!d) {
        return ST_ERR_NOT_FOUND;
    }
    *out_donor = d;
    return ST_OK;
}

/* Time: O(n), Space: O(1) where n is donor count */
Status donor_service_find_by_phone(const SystemContext *ctx, const char *phone, Donor **out_donor) {
    if (!ctx || !phone || !out_donor) return ST_ERR_INVALID;
    *out_donor = NULL;

    Donor *d = donor_list_find_by_phone(ctx->donor_list_head, phone);
    if (!d) {
        return ST_ERR_NOT_FOUND;
    }
    *out_donor = d;
    return ST_OK;
}

/* Time: O(n), Space: O(n) recursion stack */
void donor_service_display_all(const SystemContext *ctx) {
    if (!ctx) return;

    printf("\n===================================================================================================================\n");
    printf("                                       MASTER DONOR DIRECTORY (LIST ORDER)                                         \n");
    printf("===================================================================================================================\n");

    if (ctx->donor_count == 0 || ctx->donor_list_head == NULL) {
        printf("  [Directory is currently empty. No registered donors found.]\n");
    } else {
        donor_list_display_recursive(ctx->donor_list_head);
    }

    printf("-------------------------------------------------------------------------------------------------------------------\n");
    printf("Total Active Donors: %d (Max capacity: %d)\n", ctx->donor_count, MAX_DONORS);
    printf("===================================================================================================================\n");
}
