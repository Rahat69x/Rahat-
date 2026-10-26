#include "request.h"

/*
 * request.c - Implementation of Blood Request validation and helpers.
 */

/* Time: O(1), Space: O(1) */
const char *urgency_to_string(UrgencyLevel u) {
    switch (u) {
        case URG_NORMAL:   return "NORMAL";
        case URG_URGENT:   return "URGENT";
        case URG_CRITICAL: return "CRITICAL";
        default:           return "UNKNOWN";
    }
}

/* Time: O(1), Space: O(1) */
UrgencyLevel urgency_from_string(const char *str) {
    if (!str) return URG_NORMAL;
    if (utils_strcasecmp(str, "CRITICAL") == 0) return URG_CRITICAL;
    if (utils_strcasecmp(str, "URGENT") == 0)   return URG_URGENT;
    if (utils_strcasecmp(str, "NORMAL") == 0)   return URG_NORMAL;
    return URG_NORMAL;
}

/* Time: O(n), Space: O(1) */
int request_validate_patient_name(const char *name) {
    return utils_is_valid_text(name, MAX_NAME_LEN - 1);
}

/* Time: O(1), Space: O(1) */
int request_validate_blood_group(BloodGroup bg) {
    return (bg >= 0 && bg < BG_COUNT);
}

/* Time: O(1), Space: O(1) */
int request_validate_units(int units) {
    return (units >= 1 && units <= MAX_UNITS);
}

/* Time: O(n), Space: O(1) */
int request_validate_hospital(const char *hospital) {
    return utils_is_valid_text(hospital, MAX_HOSPITAL_LEN - 1);
}

/* Time: O(n), Space: O(1) */
int request_validate_city(const char *city) {
    return utils_is_valid_text(city, MAX_CITY_LEN - 1);
}

/* Time: O(1), Space: O(1) */
int request_validate_urgency(UrgencyLevel u) {
    return (u == URG_NORMAL || u == URG_URGENT || u == URG_CRITICAL);
}

/* Time: O(n), Space: O(1) */
Status request_validate_fields(const char *patient_name, BloodGroup bg, int units,
                               const char *hospital, const char *city, UrgencyLevel urgency) {
    if (!request_validate_patient_name(patient_name)) return ST_ERR_INVALID;
    if (!request_validate_blood_group(bg)) return ST_ERR_INVALID;
    if (!request_validate_units(units)) return ST_ERR_INVALID;
    if (!request_validate_hospital(hospital)) return ST_ERR_INVALID;
    if (!request_validate_city(city)) return ST_ERR_INVALID;
    if (!request_validate_urgency(urgency)) return ST_ERR_INVALID;
    return ST_OK;
}

/* Time: O(1), Space: O(1) */
void request_init(BloodRequest *req, int id, const char *patient_name, BloodGroup bg,
                  int units, const char *hospital, const char *city,
                  UrgencyLevel urgency, int sequence, Date date) {
    if (!req) return;
    req->id = id;
    utils_copy_string(req->patient_name, patient_name, sizeof(req->patient_name));
    req->blood_group = bg;
    req->units = units;
    utils_copy_string(req->hospital, hospital, sizeof(req->hospital));
    utils_copy_string(req->city, city, sizeof(req->city));
    req->urgency = urgency;
    req->arrival_sequence = sequence;
    req->request_date = date;
}
