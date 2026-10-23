/**
 * src/core/clock.c
 * Injectable mockable time implementation (C6).
 */

#include "clock.h"

static bool s_mock_active = false;
static time_t s_mock_time = 0;
static int s_mock_day = 1;

time_t clock_now(void) {
    if (s_mock_active) {
        return s_mock_time;
    }
    return time(NULL);
}

int sim_day(void) {
    if (s_mock_active) {
        return s_mock_day;
    }
    return 1;
}

void clock_set_mock(time_t fixed_time, int fixed_day) {
    s_mock_active = true;
    s_mock_time = fixed_time;
    s_mock_day = fixed_day;
}

void clock_clear_mock(void) {
    s_mock_active = false;
}

void clock_advance_days(int days) {
    if (s_mock_active) {
        s_mock_day += days;
        s_mock_time += (time_t)days * 86400;
    }
}

void clock_advance_seconds(int seconds) {
    if (s_mock_active) {
        s_mock_time += seconds;
    }
}
