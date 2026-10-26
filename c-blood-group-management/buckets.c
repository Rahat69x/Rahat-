#include "buckets.h"

/*
 * buckets.c - Implementation of blood-group classified buckets index.
 */

/* Time: O(BG_COUNT), Space: O(1) */
void buckets_init(BloodGroupBuckets *b) {
    if (!b) return;
    for (int i = 0; i < BG_COUNT; i++) {
        b->heads[i] = NULL;
        b->counts[i] = 0;
    }
}

/* Time: O(1), Space: O(1) heap allocation */
Status buckets_insert(BloodGroupBuckets *b, Donor *donor) {
    if (!b || !donor || donor->blood_group < 0 || donor->blood_group >= BG_COUNT) {
        return ST_ERR_INVALID;
    }

    /* 1. Prepare phase: allocate BucketNode */
    BucketNode *node = (BucketNode *)malloc(sizeof(BucketNode));
    if (!node) {
        return ST_ERR_NOMEM;
    }

    node->donor = donor;

    /* 2. Commit phase: prepend to target bucket chain */
    node->next = b->heads[donor->blood_group];
    b->heads[donor->blood_group] = node;
    b->counts[donor->blood_group]++;

    return ST_OK;
}

/* Time: O(k) where k is donors in bucket, Space: O(1) */
Status buckets_remove(BloodGroupBuckets *b, Donor *donor) {
    if (!b || !donor || donor->blood_group < 0 || donor->blood_group >= BG_COUNT) {
        return ST_ERR_INVALID;
    }

    int bg = donor->blood_group;
    BucketNode *curr = b->heads[bg];
    BucketNode *prev = NULL;

    while (curr) {
        if (curr->donor == donor) {
            /* Unlink node from bucket chain */
            if (prev == NULL) {
                b->heads[bg] = curr->next;
            } else {
                prev->next = curr->next;
            }
            free(curr);
            b->counts[bg]--;
            return ST_OK;
        }
        prev = curr;
        curr = curr->next;
    }

    return ST_ERR_NOT_FOUND;
}

/* Time: O(k), Space: O(1) */
Status buckets_move(BloodGroupBuckets *b, Donor *donor, BloodGroup old_bg, BloodGroup new_bg) {
    if (!b || !donor || old_bg < 0 || old_bg >= BG_COUNT || new_bg < 0 || new_bg >= BG_COUNT) {
        return ST_ERR_INVALID;
    }

    if (old_bg == new_bg) {
        return ST_OK;
    }

    /* 1. Temporarily treat donor as old_bg to unlink from old bucket */
    donor->blood_group = old_bg;
    Status st = buckets_remove(b, donor);
    if (st != ST_OK) {
        return st;
    }

    /* 2. Set new_bg and insert into new bucket */
    donor->blood_group = new_bg;
    st = buckets_insert(b, donor);
    if (st != ST_OK) {
        /* Rollback: re-insert into old bucket */
        donor->blood_group = old_bg;
        buckets_insert(b, donor);
        return st;
    }

    return ST_OK;
}

/* Time: O(1), Space: O(1) */
const BucketNode *buckets_get_head(const BloodGroupBuckets *b, BloodGroup bg) {
    if (!b || bg < 0 || bg >= BG_COUNT) {
        return NULL;
    }
    return b->heads[bg];
}

/* Time: O(1), Space: O(1) */
int buckets_get_count(const BloodGroupBuckets *b, BloodGroup bg) {
    if (!b || bg < 0 || bg >= BG_COUNT) {
        return 0;
    }
    return b->counts[bg];
}

/* Time: O(BG_COUNT + n), Space: O(1) */
void buckets_free_all(BloodGroupBuckets *b) {
    if (!b) return;
    for (int i = 0; i < BG_COUNT; i++) {
        BucketNode *curr = b->heads[i];
        while (curr) {
            BucketNode *next_node = curr->next;
            free(curr);
            curr = next_node;
        }
        b->heads[i] = NULL;
        b->counts[i] = 0;
    }
}
