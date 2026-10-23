/**
 * include/clock.h
 * Injectable time abstraction layer (C6).
 * Calls to clock_now() and sim_day() replace direct calls to time().
 */

#ifndef CLOCK_H
#define CLOCK_H

#include <time.h>
#include <stdbool.h>

time_t clock_now(void);
int sim_day(void);

void clock_set_mock(time_t fixed_time, int fixed_day);
void clock_clear_mock(void);
void clock_advance_days(int days);
void clock_advance_seconds(int seconds);

#endif /* CLOCK_H */
