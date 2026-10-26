#include "hashtable.h"

/*
 * hashtable.c - Implementation of separate chaining hash table for donor phone index.
 */

/* Time: O(HASH_SIZE), Space: O(1) */
void ht_init(HashTable *ht) {
    if (!ht) return;
    for (int i = 0; i < HASH_SIZE; i++) {
        ht->buckets[i] = NULL;
    }
    ht->count = 0;
}

/* Time: O(p), Space: O(1) where p is phone string length */
unsigned int ht_hash(const char *phone) {
    if (!phone) return 0;
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*phone++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return (unsigned int)(hash % HASH_SIZE);
}

/* Time: O(1) average, O(n) worst; Space: O(1) heap allocation */
Status ht_insert(HashTable *ht, Donor *donor) {
    if (!ht || !donor || donor->phone[0] == '\0') {
        return ST_ERR_INVALID;
    }

    unsigned int idx = ht_hash(donor->phone);

    /* 1. Check for duplicate key in collision chain */
    HashNode *curr = ht->buckets[idx];
    while (curr) {
        if (strcmp(curr->phone, donor->phone) == 0) {
            return ST_ERR_DUPLICATE;
        }
        curr = curr->next;
    }

    /* 2. Prepare phase: preallocate new HashNode */
    HashNode *node = (HashNode *)malloc(sizeof(HashNode));
    if (!node) {
        return ST_ERR_NOMEM;
    }

    utils_copy_string(node->phone, donor->phone, sizeof(node->phone));
    node->donor = donor;

    /* 3. Commit phase: prepend to bucket collision list */
    node->next = ht->buckets[idx];
    ht->buckets[idx] = node;
    ht->count++;

    return ST_OK;
}

/* Time: O(1) average, O(n) worst; Space: O(1) */
Donor *ht_search(const HashTable *ht, const char *phone) {
    if (!ht || !phone) return NULL;
    unsigned int idx = ht_hash(phone);

    HashNode *curr = ht->buckets[idx];
    while (curr) {
        if (strcmp(curr->phone, phone) == 0) {
            return curr->donor;
        }
        curr = curr->next;
    }
    return NULL;
}

/* Time: O(1) average, O(n) worst; Space: O(1) */
Status ht_delete(HashTable *ht, const char *phone) {
    if (!ht || !phone) return ST_ERR_INVALID;
    unsigned int idx = ht_hash(phone);

    HashNode *curr = ht->buckets[idx];
    HashNode *prev = NULL;

    while (curr) {
        if (strcmp(curr->phone, phone) == 0) {
            /* Unlink node from collision list */
            if (prev == NULL) {
                ht->buckets[idx] = curr->next;
            } else {
                prev->next = curr->next;
            }
            free(curr);
            ht->count--;
            return ST_OK;
        }
        prev = curr;
        curr = curr->next;
    }

    return ST_ERR_NOT_FOUND;
}

/* Time: O(HASH_SIZE + n), Space: O(1) */
void ht_free_all(HashTable *ht) {
    if (!ht) return;
    for (int i = 0; i < HASH_SIZE; i++) {
        HashNode *curr = ht->buckets[i];
        while (curr) {
            HashNode *next_node = curr->next;
            free(curr);
            curr = next_node;
        }
        ht->buckets[i] = NULL;
    }
    ht->count = 0;
}
