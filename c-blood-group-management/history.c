#include "history.h"

/*
 * history.c - Implementation of Feature F2: Fulfilment History with Doubly Linked List.
 */

/* Time: O(1), Space: O(1) */
void history_init(HistoryList *h) {
    if (!h) return;
    h->head = NULL;
    h->tail = NULL;
    h->count = 0;
}

/* Time: O(1), Space: O(1) */
Status history_append(HistoryList *h, const FulfilmentRecord *rec) {
    if (!h || !rec) {
        return ST_ERR_INVALID;
    }

    HistoryNode *node = (HistoryNode *)malloc(sizeof(HistoryNode));
    if (!node) {
        return ST_ERR_NOMEM;
    }

    node->record = *rec;
    node->next = NULL;
    node->prev = h->tail;

    if (h->tail == NULL) {
        h->head = node;
        h->tail = node;
    } else {
        h->tail->next = node;
        h->tail = node;
    }
    h->count++;

    /* Enforce bounded depth: evict oldest at head in O(1) */
    if (h->count > MAX_HISTORY) {
        HistoryNode *old_head = h->head;
        h->head = old_head->next;
        if (h->head) {
            h->head->prev = NULL;
        } else {
            h->tail = NULL;
        }
        free(old_head);
        h->count--;
    }

    return ST_OK;
}

/* Time: O(1), Space: O(1) */
Status history_remove_tail(HistoryList *h, FulfilmentRecord *out_rec) {
    if (!h || !out_rec) return ST_ERR_INVALID;
    if (h->tail == NULL || h->count == 0) {
        return ST_ERR_EMPTY;
    }

    HistoryNode *last = h->tail;
    *out_rec = last->record;

    h->tail = last->prev;
    if (h->tail) {
        h->tail->next = NULL;
    } else {
        h->head = NULL;
    }
    h->count--;

    free(last);
    return ST_OK;
}

/* Time: O(page_size), Space: O(1) */
void history_display_page(const HistoryList *h, int page_index, int page_size) {
    if (!h || page_size <= 0) return;

    int total_pages = (h->count + page_size - 1) / page_size;
    if (total_pages == 0) total_pages = 1;

    printf("\n===================================================================================================================\n");
    printf("                                   FULFILMENT HISTORY (PAGE %d OF %d | TOTAL: %d)                                    \n",
           page_index + 1, total_pages, h->count);
    printf("===================================================================================================================\n");

    if (h->count == 0 || h->tail == NULL) {
        printf("  [No blood request fulfillments recorded yet.]\n");
        printf("===================================================================================================================\n");
        return;
    }

    /* Start from tail (most recent) and skip to target page */
    const HistoryNode *curr = h->tail;
    int skip = page_index * page_size;
    for (int i = 0; i < skip && curr != NULL; i++) {
        curr = curr->prev;
    }

    int displayed = 0;
    while (curr && displayed < page_size) {
        const FulfilmentRecord *r = &curr->record;
        char d_str[16];
        date_format(r->fulfillment_date, d_str, sizeof(d_str));

        printf("+-----------------------------------------------------------------------------------------------------------------+\n");
        printf("| Request #%-4d | Patient: %-20s | Group: %-3s | Units: %d | Date: %-10s                          |\n",
               r->request_id, r->patient_name, bloodgroup_to_string(r->blood_group), r->units, d_str);
        printf("| Location: %s, %s\n", r->hospital, r->patient_city);
        printf("| Matched Donors (%d unit%s):\n", r->units, r->units > 1 ? "s" : "");
        for (int i = 0; i < r->units; i++) {
            if (r->donor_distances_km[i] >= INF_KM) {
                printf("|   [%d] Donor ID: #%-4d | %-20s | City: %-15s | Distance: Unreachable / Unknown\n",
                       i + 1, r->donor_ids[i], r->donor_names[i], r->donor_cities[i]);
            } else {
                printf("|   [%d] Donor ID: #%-4d | %-20s | City: %-15s | Distance: %d km\n",
                       i + 1, r->donor_ids[i], r->donor_names[i], r->donor_cities[i], r->donor_distances_km[i]);
            }
        }
        printf("+-----------------------------------------------------------------------------------------------------------------+\n");

        curr = curr->prev; /* Browse backwards towards older records */
        displayed++;
    }

    printf("===================================================================================================================\n");
}

/* Time: O(n), Space: O(1) */
void history_free_all(HistoryList *h) {
    if (!h) return;
    HistoryNode *curr = h->head;
    while (curr) {
        HistoryNode *next_node = curr->next;
        free(curr);
        curr = next_node;
    }
    h->head = NULL;
    h->tail = NULL;
    h->count = 0;
}
