#ifndef DATE_H
#define DATE_H

#include "common.h"

/*
 * date.h - Strict ISO-8601 calendar date representation (YYYY-MM-DD)
 * and integer days-from-civil / civil-from-days arithmetic.
 *
 * Adheres strictly to Blueprint v3 Rule 6:
 *  - Strict 10-char format: YYYY-MM-DD
 *  - Sentinel date 0000-00-00 represents "never donated"
 *  - Users input the word 'never' for it; file loader accepts literal sentinel
 *  - Pure integer days-from-civil and civil-from-days math (no mktime, DST-free)
 *  - Supports calculating 'today minus N days' for sample data and date generation
 *  - Rejects years before 1900 and any date after 'today'
 */

typedef struct Date {
    int year;
    int month;
    int day;
} Date;

/* Construct a Date struct */
Date date_create(int year, int month, int day);

/* Returns the sentinel date representing 'never donated' (0000-00-00) */
Date date_never(void);

/* Check if date is the sentinel 'never donated' value */
int date_is_never(Date d);

/* Leap year check (divisible by 4, except centuries unless divisible by 400) */
int date_is_leap_year(int year);

/* Days in a given calendar month (1-12) */
int date_days_in_month(int year, int month);

/* Compares two dates: returns -1 if d1 < d2, 0 if equal, +1 if d1 > d2 */
int date_compare(Date d1, Date d2);

/*
 * Base parser with toggle for literal sentinel.
 * Parses YYYY-MM-DD, verifies range, leap years, and ensures date <= today.
 * If allow_sentinel_literal == 1, also accepts "0000-00-00".
 * Always accepts "never" (case-insensitive).
 */
Status date_parse(const char *str, Date *out_date, Date today, int allow_sentinel_literal);

/* User-facing date parser: accepts YYYY-MM-DD or the keyword 'never' */
Status date_parse_user(const char *str, Date *out_date, Date today);

/* Storage file date parser: accepts YYYY-MM-DD, 0000-00-00, or 'never' */
Status date_parse_storage(const char *str, Date *out_date, Date today);

/* Formats a Date struct into a YYYY-MM-DD string */
void date_format(Date d, char *buf, size_t size);

/* Converts a civil calendar date to day count relative to 1970-01-01 */
long date_to_days_from_civil(Date d);

/* Converts day count relative to 1970-01-01 back to a civil calendar date */
Date date_from_days_to_civil(long z);

/* Computes difference in whole calendar days: (d2 - d1); returns -1 if either is sentinel */
int date_days_between(Date d1, Date d2);

/* Calculates a civil date exactly N days before 'today' using integer math */
Date date_days_ago(Date today, int n);

/* Retrieves current local date from system clock; handles NULL from localtime */
Date date_get_today(void);

#endif /* DATE_H */
