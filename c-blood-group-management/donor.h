#ifndef DONOR_H
#define DONOR_H

#include "common.h"
#include "date.h"
#include "bloodgroup.h"
#include "utils.h"

/*
 * donor.h - Donor domain model and master linked list data structure.
 * Layer 3 module: works strictly on its own structures and NEVER includes context.h.
 * The linked list is the master storage and single owner of Donor memory.
 */

typedef struct Donor {
    int id;                     /* Auto-increment, unique, never reused */
    char name[MAX_NAME_LEN];    /* Max 49 chars + null, printable ASCII 32-126, no '|' */
    int age;                    /* 18 to 65 inclusive */
    char gender;                /* 'M', 'F', or 'O' */
    BloodGroup blood_group;     /* Enum 0..7 (A+, A-, B+, B-, AB+, AB-, O+, O-) */
    char phone[MAX_PHONE_LEN];  /* 10-15 numeric digits, unique */
    char city[MAX_CITY_LEN];    /* Max 29 chars + null, printable ASCII 32-126, no '|' */
    Date last_donation;         /* Last donation date (sentinel 0000-00-00 if never) */
    int available;              /* 1 = willing to donate, 0 = temporarily unavailable */
    struct Donor *next;         /* Pointer to next node in master singly linked list */
} Donor;

/* --- Field Validators --- */
int donor_validate_name(const char *name);
int donor_validate_age(int age);
int donor_validate_gender(char gender);
int donor_validate_blood_group(BloodGroup bg);
int donor_validate_phone(const char *phone);
int donor_validate_city(const char *city);
int donor_validate_last_donation(Date last_donation, Date today);
int donor_validate_available(int available);

/*
 * Validates all donor fields together.
 * Returns ST_OK on success, ST_ERR_INVALID on any validation failure.
 */
Status donor_validate_fields(const char *name, int age, char gender,
                             BloodGroup bg, const char *phone, const char *city,
                             Date last_donation, int available, Date today);

/* --- Lifecycle & Memory Management (Prepare / Commit Split) --- */

/*
 * Preallocates a new Donor node on the heap and initializes its fields.
 * Returns ST_OK on success, ST_ERR_NOMEM if malloc fails, ST_ERR_INVALID on bad args.
 * Memory ownership is passed to caller.
 */
Status donor_new(Donor **out_donor, int id, const char *name, int age, char gender,
                 BloodGroup bg, const char *phone, const char *city,
                 Date last_donation, int available);

/*
 * Deallocates a single Donor node.
 */
void donor_free(Donor *donor);

/*
 * Appends a preallocated Donor to the tail of the master singly linked list.
 * Cannot fail (pointer linking only).
 */
void donor_list_append(Donor **head, Donor **tail, Donor *donor);

/*
 * Unlinks a Donor from the singly linked list by pointer.
 * Cannot fail. Does NOT free the Donor node.
 */
void donor_list_unlink(Donor **head, Donor **tail, Donor *donor);

/*
 * Deallocates every Donor in the linked list and clears head/tail to NULL.
 */
void donor_list_free_all(Donor **head, Donor **tail);

/*
 * Recursive display of donors in list order.
 * Big-O: Time O(n), Space O(n) recursion.
 */
void donor_list_display_recursive(const Donor *donor);

/*
 * Sequential linear searches on the list (used in Phase 1 before BST and Hash Table).
 */
Donor *donor_list_find_by_id(Donor *head, int id);
Donor *donor_list_find_by_phone(Donor *head, const char *phone);

#endif /* DONOR_H */
