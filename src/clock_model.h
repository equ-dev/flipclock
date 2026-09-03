#ifndef FLIPCLOCK_CLOCK_MODEL_H
#define FLIPCLOCK_CLOCK_MODEL_H

#include <stddef.h>
#include <time.h>

/*
 * ClockTime holds the display-ready, 12-hour-clock representation of a
 * moment in time. It is deliberately decoupled from time()/localtime()
 * so the conversion logic can be unit-tested with arbitrary struct tm
 * inputs, including edge cases (midnight, noon) without depending on
 * the wall clock.
 */
typedef struct {
  int display_hour;   /* 1-12 */
  int minute;          /* 0-59 */
  int second;          /* 0-59 */
  int is_pm;            /* 0 = AM, 1 = PM */
} ClockTime;

/* Converts a standard broken-down time (24-hour tm_hour, 0-23) into the
 * 12-hour ClockTime representation used for display. */
void clock_time_from_tm (const struct tm *tm, ClockTime *out);

/* Convenience wrapper: reads the current local time and converts it. */
void clock_time_now (ClockTime *out);

/* Formats "H:MM" (no leading zero on the hour) into buf. Returns the
 * number of characters written (excluding the null terminator), or -1
 * if buf is too small. */
int clock_time_format_hm (const ClockTime *ct, char *buf, size_t buflen);

/* Returns a static "AM" or "PM" string. Never returns NULL. */
const char *clock_time_ampm_string (const ClockTime *ct);

#endif /* FLIPCLOCK_CLOCK_MODEL_H */
