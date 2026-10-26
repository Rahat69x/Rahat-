#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "common.h"
#include "donor.h"

/*
 * hashtable.h - Separate chaining hash table index on Donor phone number.
 * Layer 3 structure module: does NOT own Donor memory.
 * HashNode points to Donor; freeing a HashNode does NOT free the Donor.
 * Buckets array size is fixed at HASH_SIZE (101, prime).
 * Never includes context.h.
 */

typedef struct HashNode {
    char phone[MAX_PHONE_LEN];  /* Key: Donor phone number */
    Donor *donor;               /* Pointer to Donor owned by master linked list */
    struct HashNode *next;      /* Collision chain pointer */
} HashNode;

typedef struct HashTable {
    HashNode *buckets[HASH_SIZE]; /* Array of collision chain heads */
    int count;                    /* Total indexed elements */
} HashTable;

/*
 * Initializes hash table buckets to NULL and count to 0.
 * Time: O(HASH_SIZE), Space: O(1)
 */
void ht_init(HashTable *ht);

/*
 * Computes hash bucket index for a phone string using djb2 algorithm.
 * Time: O(p), Space: O(1) where p is phone length
 */
unsigned int ht_hash(const char *phone);

/*
 * Inserts a donor into the hash table keyed by donor->phone.
 * Checks for duplicate phone. Returns ST_OK on success, ST_ERR_DUPLICATE if key exists,
 * ST_ERR_NOMEM if allocation fails, ST_ERR_INVALID on bad arguments.
 * Time: O(1) average, O(n) worst-case collision; Space: O(1) heap node.
 */
Status ht_insert(HashTable *ht, Donor *donor);

/*
 * Searches hash table for a donor with matching phone number.
 * Returns pointer to Donor if found, or NULL if not found.
 * Time: O(1) average, O(n) worst-case collision; Space: O(1).
 */
Donor *ht_search(const HashTable *ht, const char *phone);

/*
 * Removes a donor's phone key from the hash table.
 * Frees the HashNode. Does NOT free the Donor node.
 * Returns ST_OK on success, ST_ERR_NOT_FOUND if phone not in table.
 * Time: O(1) average, O(n) worst; Space: O(1).
 */
Status ht_delete(HashTable *ht, const char *phone);

/*
 * Deallocates all HashNode instances across all buckets and resets table count to 0.
 * Does NOT deallocate the underlying Donor records.
 * Time: O(HASH_SIZE + n), Space: O(1).
 */
void ht_free_all(HashTable *ht);

#endif /* HASHTABLE_H */
