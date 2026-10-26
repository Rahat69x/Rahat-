#ifndef BUCKETS_H
#define BUCKETS_H

#include "common.h"
#include "bloodgroup.h"
#include "donor.h"

/*
 * buckets.h - Blood group classified buckets index.
 * Layer 3 structure module: does NOT own Donor memory.
 * Organizes active donors into 8 singly-linked bucket chains corresponding
 * to the 8 standard blood groups (A+, A-, B+, B-, AB+, AB-, O+, O-).
 * Enables instant O(1) access to donors of a specific blood group.
 * Never includes context.h.
 */

typedef struct BucketNode {
    Donor *donor;              /* Pointer to Donor owned by master linked list */
    struct BucketNode *next;   /* Next node in this blood group bucket */
} BucketNode;

typedef struct BloodGroupBuckets {
    BucketNode *heads[BG_COUNT]; /* Heads of the 8 blood group bucket chains */
    int counts[BG_COUNT];        /* Count of donors currently in each bucket */
} BloodGroupBuckets;

/*
 * Initializes all 8 bucket heads to NULL and counts to 0.
 * Time: O(BG_COUNT), Space: O(1)
 */
void buckets_init(BloodGroupBuckets *b);

/*
 * Inserts a donor pointer into the bucket corresponding to donor->blood_group.
 * Preallocates BucketNode. Returns ST_OK on success, ST_ERR_NOMEM if allocation fails,
 * ST_ERR_INVALID on bad arguments.
 * Time: O(1), Space: O(1) heap allocation.
 */
Status buckets_insert(BloodGroupBuckets *b, Donor *donor);

/*
 * Removes a donor pointer from the bucket corresponding to donor->blood_group.
 * Frees the BucketNode. Does NOT free the Donor node.
 * Returns ST_OK if unlinked, ST_ERR_NOT_FOUND if donor not in bucket.
 * Time: O(k) where k is number of donors in that specific bucket; Space: O(1).
 */
Status buckets_remove(BloodGroupBuckets *b, Donor *donor);

/*
 * Moves a donor from an old blood group bucket to a new blood group bucket.
 * Used during donor field updates if blood group is modified.
 * Time: O(k), Space: O(1).
 */
Status buckets_move(BloodGroupBuckets *b, Donor *donor, BloodGroup old_bg, BloodGroup new_bg);

/*
 * Returns the head pointer of a specific blood group bucket chain.
 * Time: O(1), Space: O(1).
 */
const BucketNode *buckets_get_head(const BloodGroupBuckets *b, BloodGroup bg);

/*
 * Returns the count of donors currently in a specific blood group bucket.
 * Time: O(1), Space: O(1).
 */
int buckets_get_count(const BloodGroupBuckets *b, BloodGroup bg);

/*
 * Deallocates all BucketNode instances across all 8 buckets and resets counts to 0.
 * Does NOT deallocate the underlying Donor records.
 * Time: O(BG_COUNT + n), Space: O(1).
 */
void buckets_free_all(BloodGroupBuckets *b);

#endif /* BUCKETS_H */
