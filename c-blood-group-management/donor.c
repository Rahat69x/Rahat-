#include "donor.h"

/*
 * donor.c - Implementation of Donor validation, memory lifecycle,
 * and master singly linked list operations.
 */

/* Time: O(n), Space: O(1) where n is name length */
int donor_validate_name(const char *name) {
    return utils_is_valid_text(name, MAX_NAME_LEN - 1);
}

/* Time: O(1), Space: O(1) */
int donor_validate_age(int age) {
    return utils_is_valid_age(age);
}

/* Time: O(1), Space: O(1) */
int donor_validate_gender(char gender) {
    char g = (char)toupper((unsigned char)gender);
    return (g == 'M' || g == 'F' || g == 'O');
}

/* Time: O(1), Space: O(1) */
int donor_validate_blood_group(BloodGroup bg) {
    return (bg >= 0 && bg < BG_COUNT);
}

/* Time: O(p), Space: O(1) where p is phone length */
int donor_validate_phone(const char *phone) {
    return utils_is_valid_phone(phone);
}

/* Time: O(c), Space: O(1) where c is city length */
int donor_validate_city(const char *city) {
    return utils_is_valid_text(city, MAX_CITY_LEN - 1);
}

/* Time: O(1), Space: O(1) */
int donor_validate_last_donation(Date last_donation, Date today) {
    if (date_is_never(last_donation)) {
        return 1;
    }
    if (last_donation.year < 1900) {
        return 0;
    }
    if (last_donation.month < 1 || last_donation.month > 12) {
        return 0;
    }
    int max_days = date_days_in_month(last_donation.year, last_donation.month);
    if (last_donation.day < 1 || last_donation.day > max_days) {
        return 0;
    }
    if (!date_is_never(today)) {
        if (date_compare(last_donation, today) > 0) {
            return 0; /* Future dates relative to today are prohibited */
        }
    }
    return 1;
}

/* Time: O(1), Space: O(1) */
int donor_validate_available(int available) {
    return (available == 0 || available == 1);
}

/* Time: O(n + c + p), Space: O(1) */
Status donor_validate_fields(const char *name, int age, char gender,
                             BloodGroup bg, const char *phone, const char *city,
                             Date last_donation, int available, Date today) {
    if (!donor_validate_name(name)) return ST_ERR_INVALID;
    if (!donor_validate_age(age)) return ST_ERR_INVALID;
    if (!donor_validate_gender(gender)) return ST_ERR_INVALID;
    if (!donor_validate_blood_group(bg)) return ST_ERR_INVALID;
    if (!donor_validate_phone(phone)) return ST_ERR_INVALID;
    if (!donor_validate_city(city)) return ST_ERR_INVALID;
    if (!donor_validate_last_donation(last_donation, today)) return ST_ERR_INVALID;
    if (!donor_validate_available(available)) return ST_ERR_INVALID;
    return ST_OK;
}

/* Time: O(1), Space: O(1) heap allocation */
Status donor_new(Donor **out_donor, int id, const char *name, int age, char gender,
                 BloodGroup bg, const char *phone, const char *city,
                 Date last_donation, int available) {
    if (!out_donor || id <= 0) {
        return ST_ERR_INVALID;
    }
    *out_donor = NULL;

    Donor *d = (Donor *)malloc(sizeof(Donor));
    if (!d) {
        return ST_ERR_NOMEM;
    }

    d->id = id;
    utils_copy_string(d->name, name, sizeof(d->name));
    d->age = age;
    d->gender = (char)toupper((unsigned char)gender);
    d->blood_group = bg;
    utils_copy_string(d->phone, phone, sizeof(d->phone));
    utils_copy_string(d->city, city, sizeof(d->city));
    d->last_donation = last_donation;
    d->available = available;
    d->next = NULL;

    *out_donor = d;
    return ST_OK;
}

/* Time: O(1), Space: O(1) */
void donor_free(Donor *donor) {
    if (donor) {
        free(donor);
    }
}

/* Time: O(1), Space: O(1) */
void donor_list_append(Donor **head, Donor **tail, Donor *donor) {
    if (!head || !tail || !donor) return;
    donor->next = NULL;

    if (*tail == NULL) {
        *head = donor;
        *tail = donor;
    } else {
        (*tail)->next = donor;
        *tail = donor;
    }
}

/* Time: O(n), Space: O(1) */
void donor_list_unlink(Donor **head, Donor **tail, Donor *donor) {
    if (!head || !tail || !*head || !donor) return;

    if (*head == donor) {
        *head = donor->next;
        if (*tail == donor) {
            *tail = NULL;
        }
        donor->next = NULL;
        return;
    }

    Donor *prev = *head;
    while (prev->next && prev->next != donor) {
        prev = prev->next;
    }

    if (prev->next == donor) {
        prev->next = donor->next;
        if (*tail == donor) {
            *tail = prev;
        }
        donor->next = NULL;
    }
}

/* Time: O(n), Space: O(1) */
void donor_list_free_all(Donor **head, Donor **tail) {
    if (!head || !tail) return;
    Donor *current = *head;
    while (current) {
        Donor *next_node = current->next;
        free(current);
        current = next_node;
    }
    *head = NULL;
    *tail = NULL;
}

/* Time: O(n), Space: O(n) recursion stack (Section 12 requirement) */
void donor_list_display_recursive(const Donor *donor) {
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

    donor_list_display_recursive(donor->next);
}

/* Time: O(n), Space: O(1) */
Donor *donor_list_find_by_id(Donor *head, int id) {
    Donor *curr = head;
    while (curr) {
        if (curr->id == id) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

/* Time: O(n), Space: O(1) */
Donor *donor_list_find_by_phone(Donor *head, const char *phone) {
    if (!phone) return NULL;
    Donor *curr = head;
    while (curr) {
        if (strcmp(curr->phone, phone) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}
