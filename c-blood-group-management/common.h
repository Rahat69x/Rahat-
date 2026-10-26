#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/*
 * common.h - Global status codes, domain constants, and status message resolver.
 * Layer 1 module: included by foundational and domain modules.
 * Only the five permitted C99 headers are used across the codebase:
 * stdio.h, stdlib.h, string.h, ctype.h, time.h.
 */

/* Unified status return codes */
typedef enum {
    ST_OK              =  0,  /* Operation completed successfully */
    ST_ERR_NOMEM       = -1,  /* Dynamic memory allocation (malloc/realloc) failed */
    ST_ERR_INVALID     = -2,  /* Input validation failed or arguments out of bounds */
    ST_ERR_NOT_FOUND   = -3,  /* Specified entity or record was not found */
    ST_ERR_DUPLICATE   = -4,  /* Unique key constraint violated (e.g. duplicate phone) */
    ST_ERR_EMPTY       = -5,  /* Data structure is empty */
    ST_ERR_STATE       = -6,  /* Operation invalid in current state / user canceled */
    ST_ERR_NO_DONORS   = -7,  /* Insufficient eligible compatible donors found */
    ST_ERR_IO          = -8,  /* File or stream input/output error */
    ST_ERR_FORMAT      = -9   /* File format, checksum, or syntax validation error */
} Status;

/* Global capacity and domain limits */
#define MAX_DONORS              10000
#define MAX_PENDING_REQUESTS    1000
#define UNDO_DEPTH              20
#define HASH_SIZE               101
#define MIN_DONATION_GAP_DAYS   90

#define MAX_NAME_LEN            50
#define MAX_PHONE_LEN           16
#define MAX_CITY_LEN            30
#define MAX_HOSPITAL_LEN        50

#define MIN_DONOR_AGE           18
#define MAX_DONOR_AGE           65

#define MIN_PHONE_DIGITS        10
#define MAX_PHONE_DIGITS        15

/* Returns a human-readable description for a status code */
const char *status_message(Status s);

#endif /* COMMON_H */
