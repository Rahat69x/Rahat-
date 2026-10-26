#include "common.h"

/*
 * common.c - Implementation of global status code descriptions.
 */

/* Time: O(1), Space: O(1) */
const char *status_message(Status s) {
    switch (s) {
        case ST_OK:
            return "Success";
        case ST_ERR_NOMEM:
            return "Out of memory: memory allocation failed";
        case ST_ERR_INVALID:
            return "Invalid argument or input out of permitted bounds";
        case ST_ERR_NOT_FOUND:
            return "Record not found";
        case ST_ERR_DUPLICATE:
            return "Duplicate key rejected (must be unique)";
        case ST_ERR_EMPTY:
            return "Structure or container is empty";
        case ST_ERR_STATE:
            return "Invalid operational state or canceled by user";
        case ST_ERR_NO_DONORS:
            return "Insufficient eligible compatible donors found";
        case ST_ERR_IO:
            return "I/O stream or file access error";
        case ST_ERR_FORMAT:
            return "Invalid data format or checksum verification failure";
        case ST_ERR_LIMIT:
            return "Capacity or configuration limit reached";
        default:
            return "Unknown status code";
    }
}
