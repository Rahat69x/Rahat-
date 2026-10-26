#ifndef REQUEST_H
#define REQUEST_H

#include "common.h"
#include "date.h"
#include "bloodgroup.h"
#include "utils.h"

/*
 * request.h - Blood Request domain model and validation.
 * Layer 3 module: works strictly on its own structures and NEVER includes context.h.
 */

typedef enum {
    URG_NORMAL   = 0,
    URG_URGENT   = 1,
    URG_CRITICAL = 2
} UrgencyLevel;

typedef struct BloodRequest {
    int id;                                 /* Unique request ID, auto-increment */
    char patient_name[MAX_NAME_LEN];        /* Printable ASCII, max 49 chars, no '|' */
    BloodGroup blood_group;                 /* Blood group required (0..7) */
    int units;                              /* Number of units requested (1 to MAX_UNITS) */
    char hospital[MAX_HOSPITAL_LEN];        /* Hospital name, max 49 chars, no '|' */
    char city[MAX_CITY_LEN];                /* Patient city for F1 distance ranking, no '|' */
    UrgencyLevel urgency;                   /* Urgency level (0=Normal, 1=Urgent, 2=Critical) */
    int arrival_sequence;                   /* Global sequence counter for FIFO tie-breaking */
    Date request_date;                      /* Date the request was submitted */
} BloodRequest;

/* --- Urgency Converters --- */
const char *urgency_to_string(UrgencyLevel u);
UrgencyLevel urgency_from_string(const char *str);

/* --- Field Validators --- */
int request_validate_patient_name(const char *name);
int request_validate_blood_group(BloodGroup bg);
int request_validate_units(int units);
int request_validate_hospital(const char *hospital);
int request_validate_city(const char *city);
int request_validate_urgency(UrgencyLevel u);

/*
 * Validates all blood request fields together.
 * Returns ST_OK on success, ST_ERR_INVALID on any validation failure.
 */
Status request_validate_fields(const char *patient_name, BloodGroup bg, int units,
                               const char *hospital, const char *city, UrgencyLevel urgency);

/*
 * Initializes a BloodRequest struct with the given validated parameters.
 */
void request_init(BloodRequest *req, int id, const char *patient_name, BloodGroup bg,
                  int units, const char *hospital, const char *city,
                  UrgencyLevel urgency, int sequence, Date date);

#endif /* REQUEST_H */
