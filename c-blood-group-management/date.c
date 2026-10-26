#include "date.h"

/*
 * date.c - Implementation of strict calendar date validation, days-from-civil,
 * and civil-from-days integer arithmetic.
 */

/* Time: O(1), Space: O(1) */
Date date_create(int year, int month, int day) {
    Date d;
    d.year = year;
    d.month = month;
    d.day = day;
    return d;
}

/* Time: O(1), Space: O(1) */
Date date_never(void) {
    return date_create(0, 0, 0);
}

/* Time: O(1), Space: O(1) */
int date_is_never(Date d) {
    return (d.year == 0 && d.month == 0 && d.day == 0);
}

/* Time: O(1), Space: O(1) */
int date_is_leap_year(int year) {
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        return 1;
    }
    return 0;
}

/* Time: O(1), Space: O(1) */
int date_days_in_month(int year, int month) {
    static const int days_per_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month < 1 || month > 12) {
        return 0;
    }
    if (month == 2 && date_is_leap_year(year)) {
        return 29;
    }
    return days_per_month[month - 1];
}

/* Time: O(1), Space: O(1) */
int date_compare(Date d1, Date d2) {
    if (d1.year != d2.year) {
        return (d1.year < d2.year) ? -1 : 1;
    }
    if (d1.month != d2.month) {
        return (d1.month < d2.month) ? -1 : 1;
    }
    if (d1.day != d2.day) {
        return (d1.day < d2.day) ? -1 : 1;
    }
    return 0;
}

/* Time: O(1), Space: O(1) */
Status date_parse(const char *str, Date *out_date, Date today, int allow_sentinel_literal) {
    if (!str || !out_date) {
        return ST_ERR_INVALID;
    }

    /* Check for case-insensitive keyword 'never' */
    if ((str[0] == 'n' || str[0] == 'N') &&
        (str[1] == 'e' || str[1] == 'E') &&
        (str[2] == 'v' || str[2] == 'V') &&
        (str[3] == 'e' || str[3] == 'E') &&
        (str[4] == 'r' || str[4] == 'R') &&
        str[5] == '\0') {
        *out_date = date_never();
        return ST_OK;
    }

    /* Strict 10-character length requirement */
    if (strlen(str) != 10) {
        return ST_ERR_INVALID;
    }

    /* Fixed delimiter check */
    if (str[4] != '-' || str[7] != '-') {
        return ST_ERR_INVALID;
    }

    /* Verify all non-delimiter positions are digits */
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) continue;
        if (!isdigit((unsigned char)str[i])) {
            return ST_ERR_INVALID;
        }
    }

    /* Check for literal sentinel 0000-00-00 */
    if (strcmp(str, "0000-00-00") == 0) {
        if (allow_sentinel_literal) {
            *out_date = date_never();
            return ST_OK;
        }
        /* Users must enter 'never', not the literal sentinel */
        return ST_ERR_INVALID;
    }

    int y = (str[0] - '0') * 1000 + (str[1] - '0') * 100 + (str[2] - '0') * 10 + (str[3] - '0');
    int m = (str[5] - '0') * 10 + (str[6] - '0');
    int d = (str[8] - '0') * 10 + (str[9] - '0');

    /* Rule 6: Reject years before 1900 */
    if (y < 1900) {
        return ST_ERR_INVALID;
    }

    /* Validate month range */
    if (m < 1 || m > 12) {
        return ST_ERR_INVALID;
    }

    /* Validate day within month */
    int max_days = date_days_in_month(y, m);
    if (d < 1 || d > max_days) {
        return ST_ERR_INVALID;
    }

    Date parsed = date_create(y, m, d);

    /* Rule 6: Reject any date after today */
    if (!date_is_never(today)) {
        if (date_compare(parsed, today) > 0) {
            return ST_ERR_INVALID;
        }
    }

    *out_date = parsed;
    return ST_OK;
}

/* Time: O(1), Space: O(1) */
Status date_parse_user(const char *str, Date *out_date, Date today) {
    return date_parse(str, out_date, today, 0);
}

/* Time: O(1), Space: O(1) */
Status date_parse_storage(const char *str, Date *out_date, Date today) {
    return date_parse(str, out_date, today, 1);
}

/* Time: O(1), Space: O(1) */
void date_format(Date d, char *buf, size_t size) {
    if (!buf || size < 11) return;
    if (date_is_never(d)) {
        snprintf(buf, size, "0000-00-00");
    } else {
        snprintf(buf, size, "%04d-%02d-%02d", d.year, d.month, d.day);
    }
}

/* Time: O(1), Space: O(1) - Pure integer Gregorian civil day calculation */
long date_to_days_from_civil(Date d) {
    if (date_is_never(d)) return -1;
    long y = (long)d.year;
    long m = (long)d.month;
    long day = (long)d.day;
    if (m <= 2) {
        y -= 1;
        m += 9;
    } else {
        m -= 3;
    }
    long era = (y >= 0 ? y : y - 399) / 400;
    unsigned long yoe = (unsigned long)(y - era * 400);
    unsigned long doy = (unsigned long)((153 * m + 2) / 5 + day - 1);
    unsigned long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long)doe - 719468;
}

/* Time: O(1), Space: O(1) - Pure integer civil calendar calculation from day count */
Date date_from_days_to_civil(long z) {
    z += 719468;
    long era = (z >= 0 ? z : z - 146096) / 146097;
    unsigned long doe = (unsigned long)(z - era * 146097);
    unsigned long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long y = (long)yoe + era * 400;
    unsigned long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    unsigned long mp = (5 * doy + 2) / 153;
    unsigned long d = doy - (153 * mp + 2) / 5 + 1;
    unsigned long m = mp + (mp < 10 ? 3 : -9);
    y += (m <= 2 ? 1 : 0);
    return date_create((int)y, (int)m, (int)d);
}

/* Time: O(1), Space: O(1) */
int date_days_between(Date d1, Date d2) {
    if (date_is_never(d1) || date_is_never(d2)) {
        return -1;
    }
    long jdn1 = date_to_days_from_civil(d1);
    long jdn2 = date_to_days_from_civil(d2);
    return (int)(jdn2 - jdn1);
}

/* Time: O(1), Space: O(1) */
Date date_days_ago(Date today, int n) {
    if (date_is_never(today) || n < 0) {
        return date_never();
    }
    long z = date_to_days_from_civil(today);
    return date_from_days_to_civil(z - (long)n);
}

/* Time: O(1), Space: O(1) */
Date date_get_today(void) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    if (tm_info) {
        return date_create(tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);
    }
    /* Safe fallback if localtime returns NULL */
    return date_create(2026, 1, 1);
}
