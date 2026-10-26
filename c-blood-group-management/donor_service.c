#include "donor_service.h"

/*
 * donor_service.c - Transactional donor operations implementation.
 */

/* Time: O(log n) avg, Space: O(1) */
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

    /* 3. Phone uniqueness check via Hash Table: O(1) average */
    if (ht_search(&ctx->phone_table, phone) != NULL) {
        return ST_ERR_DUPLICATE;
    }

    /* 4. Prepare phase: allocate Donor node */
    Donor *new_donor = NULL;
    st = donor_new(&new_donor, ctx->next_donor_id, name, age, gender,
                   bg, phone, city, last_donation, available);
    if (st != ST_OK) {
        return st;
    }

    /* 5. Prepare phase: insert into BST ID index: O(log n) average */
    st = bst_insert(&ctx->donor_id_bst, new_donor);
    if (st != ST_OK) {
        donor_free(new_donor);
        return st;
    }

    /* 6. Prepare phase: insert into Hash Table phone index: O(1) average */
    st = ht_insert(&ctx->phone_table, new_donor);
    if (st != ST_OK) {
        bst_delete(&ctx->donor_id_bst, new_donor->id);
        donor_free(new_donor);
        return st;
    }

    /* 7. Prepare phase: insert into 8-group classified bucket index: O(1) */
    st = buckets_insert(&ctx->bg_buckets, new_donor);
    if (st != ST_OK) {
        ht_delete(&ctx->phone_table, new_donor->phone);
        bst_delete(&ctx->donor_id_bst, new_donor->id);
        donor_free(new_donor);
        return st;
    }

    /* 8. Commit phase: link into master list and update counters (cannot fail) */
    donor_list_append(&ctx->donor_list_head, &ctx->donor_list_tail, new_donor);
    if (out_id) {
        *out_id = ctx->next_donor_id;
    }
    ctx->next_donor_id++;
    ctx->donor_count++;

    return ST_OK;
}

/* Time: O(log n) avg, Space: O(1) */
Status donor_service_update(SystemContext *ctx, int id, const char *name, int age, char gender,
                            BloodGroup bg, const char *phone, const char *city,
                            Date last_donation, int available, Date today) {
    if (!ctx || id <= 0) return ST_ERR_INVALID;

    /* 1. Find target donor via BST ID index */
    Donor *d = bst_search(ctx->donor_id_bst, id);
    if (!d) {
        return ST_ERR_NOT_FOUND;
    }

    /* 2. Validate all new fields */
    Status st = donor_validate_fields(name, age, gender, bg, phone, city,
                                      last_donation, available, today);
    if (st != ST_OK) {
        return st;
    }

    /* 3. If phone changed, verify uniqueness and update Hash Table index */
    if (strcmp(d->phone, phone) != 0) {
        if (ht_search(&ctx->phone_table, phone) != NULL) {
            return ST_ERR_DUPLICATE;
        }

        /* Update hash table: remove old phone, insert with new phone */
        ht_delete(&ctx->phone_table, d->phone);
        utils_copy_string(d->phone, phone, sizeof(d->phone));
        st = ht_insert(&ctx->phone_table, d);
        if (st != ST_OK) {
            return st;
        }
    }

    /* 4. If blood group changed, move between classified buckets */
    if (d->blood_group != bg) {
        buckets_move(&ctx->bg_buckets, d, d->blood_group, bg);
    }

    /* 5. Commit phase: mutate remaining fields (ID immutable -> BST structure preserved) */
    utils_copy_string(d->name, name, sizeof(d->name));
    d->age = age;
    d->gender = (char)toupper((unsigned char)gender);
    d->blood_group = bg;
    utils_copy_string(d->city, city, sizeof(d->city));
    d->last_donation = last_donation;
    d->available = available;

    return ST_OK;
}

/* Time: O(log n) avg, Space: O(1) */
Status donor_service_delete(SystemContext *ctx, int id) {
    if (!ctx || id <= 0) return ST_ERR_INVALID;

    /* 1. Find target donor via BST ID index */
    Donor *d = bst_search(ctx->donor_id_bst, id);
    if (!d) {
        return ST_ERR_NOT_FOUND;
    }

    /* 2. Commit phase: remove from classified buckets, Hash Table, BST index, unlink from list, free */
    buckets_remove(&ctx->bg_buckets, d);
    ht_delete(&ctx->phone_table, d->phone);
    bst_delete(&ctx->donor_id_bst, id);
    donor_list_unlink(&ctx->donor_list_head, &ctx->donor_list_tail, d);
    donor_free(d);
    ctx->donor_count--;

    return ST_OK;
}

/* Time: O(log n) avg, O(n) worst; Space: O(1) */
Status donor_service_find_by_id(const SystemContext *ctx, int id, Donor **out_donor) {
    if (!ctx || id <= 0 || !out_donor) return ST_ERR_INVALID;
    *out_donor = NULL;

    Donor *d = bst_search(ctx->donor_id_bst, id);
    if (!d) {
        return ST_ERR_NOT_FOUND;
    }
    *out_donor = d;
    return ST_OK;
}

/* Time: O(1) average, O(n) worst; Space: O(1) */
Status donor_service_find_by_phone(const SystemContext *ctx, const char *phone, Donor **out_donor) {
    if (!ctx || !phone || !out_donor) return ST_ERR_INVALID;
    *out_donor = NULL;

    Donor *d = ht_search(&ctx->phone_table, phone);
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

/* Helper callback for in-order traversal */
static void print_donor_bst_visitor(const Donor *donor, void *user_data) {
    (void)user_data;
    if (!donor) return;

    char date_buf[16];
    date_format(donor->last_donation, date_buf, sizeof(date_buf));

    printf("  ID: %-5d | Name: %-20s | Age: %2d | Gender: %c | Group: %-3s | Phone: %-15s | City: %-15s | Last: %s | Active: %s\n",
           donor->id,
           donor->name,
           donor->age,
           donor->gender,
           bloodgroup_to_string(donor->blood_group),
           donor->phone,
           donor->city,
           date_is_never(donor->last_donation) ? "never     " : date_buf,
           donor->available ? "YES" : "NO ");
}

/* Time: O(n), Space: O(h) recursion stack */
void donor_service_display_sorted_by_id(const SystemContext *ctx) {
    if (!ctx) return;

    printf("\n===================================================================================================================\n");
    printf("                                    MASTER DONOR DIRECTORY (SORTED BY ID - BST)                                    \n");
    printf("===================================================================================================================\n");

    if (ctx->donor_count == 0 || ctx->donor_id_bst == NULL) {
        printf("  [Directory is currently empty. No registered donors found.]\n");
    } else {
        bst_inorder(ctx->donor_id_bst, print_donor_bst_visitor, NULL);
    }

    printf("-------------------------------------------------------------------------------------------------------------------\n");
    printf("Total Active Donors (Indexed in BST): %d\n", bst_count(ctx->donor_id_bst));
    printf("===================================================================================================================\n");
}

/* Time: O(k), Space: O(1) */
void donor_service_display_by_blood_group(const SystemContext *ctx, BloodGroup bg) {
    if (!ctx || bg < 0 || bg >= BG_COUNT) return;

    const char *bg_str = bloodgroup_to_string(bg);
    printf("\n===================================================================================================================\n");
    printf("                                 CLASSIFIED DONOR DIRECTORY (BLOOD GROUP: %-3s)                                     \n", bg_str);
    printf("===================================================================================================================\n");

    const BucketNode *curr = buckets_get_head(&ctx->bg_buckets, bg);
    int count = buckets_get_count(&ctx->bg_buckets, bg);

    if (!curr || count == 0) {
        printf("  [No registered donors found in Blood Group %s.]\n", bg_str);
    } else {
        while (curr) {
            const Donor *d = curr->donor;
            char date_buf[16];
            date_format(d->last_donation, date_buf, sizeof(date_buf));

            printf("  ID: %-5d | Name: %-20s | Age: %2d | Gender: %c | Group: %-3s | Phone: %-15s | City: %-15s | Last: %s | Active: %s\n",
                   d->id,
                   d->name,
                   d->age,
                   d->gender,
                   bloodgroup_to_string(d->blood_group),
                   d->phone,
                   d->city,
                   date_is_never(d->last_donation) ? "never     " : date_buf,
                   d->available ? "YES" : "NO ");
            curr = curr->next;
        }
    }

    printf("-------------------------------------------------------------------------------------------------------------------\n");
    printf("Total Donors in Blood Group [%s]: %d\n", bg_str, count);
    printf("===================================================================================================================\n");
}
