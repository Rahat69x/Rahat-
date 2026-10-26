#ifndef HISTORY_H
#define HISTORY_H

#include "common.h"
#include "date.h"
#include "bloodgroup.h"
#include "utils.h"

/*
 * history.h - Feature F2: Fulfilment History with Doubly Linked List.
 * Layer 3 structure module: does NOT own Donor memory and NEVER includes context.h.
 * Bounded by MAX_HISTORY (200). When capacity is reached, the oldest node at head
 * is evicted in O(1) time. Supports bidirectional browsing and pagination.
 */

typedef struct FulfilmentRecord {
    int request_id;
    char patient_name[MAX_NAME_LEN];
    BloodGroup blood_group;
    int units;
    char hospital[MAX_HOSPITAL_LEN];
    char patient_city[MAX_CITY_LEN];
    Date fulfillment_date;
    int donor_ids[MAX_UNITS];
    char donor_names[MAX_UNITS][MAX_NAME_LEN];
    char donor_cities[MAX_UNITS][MAX_CITY_LEN];
    int donor_distances_km[MAX_UNITS];
} FulfilmentRecord;

typedef struct HistoryNode {
    FulfilmentRecord record;
    struct HistoryNode *prev;   /* Pointer to older record */
    struct HistoryNode *next;   /* Pointer to newer record */
} HistoryNode;

typedef struct HistoryList {
    HistoryNode *head;          /* Oldest fulfillment record */
    HistoryNode *tail;          /* Most recent fulfillment record */
    int count;                  /* Total records currently stored */
} HistoryList;

/*
 * Initializes an empty HistoryList.
 * Time: O(1), Space: O(1)
 */
void history_init(HistoryList *h);

/*
 * Appends a new fulfillment record to the tail of the doubly linked list.
 * If list count reaches MAX_HISTORY, evicts the oldest node at head in O(1) time.
 * Time: O(1), Space: O(1) heap allocation.
 */
Status history_append(HistoryList *h, const FulfilmentRecord *rec);

/*
 * Removes and returns the most recent record from the tail of the list.
 * Invoked during fulfillment undo operations.
 * Returns ST_OK on success, ST_ERR_EMPTY if list is empty.
 * Time: O(1), Space: O(1).
 */
Status history_remove_tail(HistoryList *h, FulfilmentRecord *out_rec);

/*
 * Displays a single page of history records starting from newest (tail) towards oldest.
 * page_index is 0-based.
 * Time: O(page_size), Space: O(1).
 */
void history_display_page(const HistoryList *h, int page_index, int page_size);

/*
 * Deallocates all HistoryNode instances and resets head/tail to NULL.
 * Time: O(n), Space: O(1).
 */
void history_free_all(HistoryList *h);

#endif /* HISTORY_H */
