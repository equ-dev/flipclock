#ifndef FLIPCLOCK_ALARM_H
#define FLIPCLOCK_ALARM_H

#include <stdbool.h>

#include "clock_model.h"

#define ALARM_SNOOZE_MINUTES 5

/*
 * AlarmState is edge-triggered: it fires exactly once on the tick where
 * the current time transitions INTO matching the alarm time, using
 * was_matching_last_check to detect that transition, rather than
 * remembering "the last total-minutes value that triggered." The
 * latter approach looks correct at first but has a real bug: since
 * time-of-day wraps every 1440 minutes, remembering a single trigger
 * value would silently prevent the alarm from ever ringing again on
 * subsequent days once that exact minute-value recurred. Edge
 * detection avoids that entirely.
 */
typedef struct {
  int hour;      /* 1-12, alarm target hour (12-hour display) */
  int minute;    /* 0-59 */
  bool is_pm;

  bool enabled;
  bool ringing;
  bool snoozed;
  int snoozed_until_total_minutes; /* valid only if snoozed */

  bool was_matching_last_check; /* for rising-edge detection */
} AlarmState;

void alarm_init (AlarmState *alarm);

/* Adjusts alarm hour by delta (wraps within 1-12). */
void alarm_adjust_hour (AlarmState *alarm, int delta);
/* Adjusts alarm minute by delta (wraps within 0-59). */
void alarm_adjust_minute (AlarmState *alarm, int delta);
void alarm_toggle_pm (AlarmState *alarm);
void alarm_set_enabled (AlarmState *alarm, bool enabled);

/* Call every tick with the current wall-clock time. Sets alarm->ringing
 * true exactly once per matching-minute entry (see rising-edge note
 * above), and clears an expired snooze by re-ringing. Pure function of
 * (alarm state, current time) -- safe and cheap to call every frame. */
void alarm_check (AlarmState *alarm, const ClockTime *now);

/* Silences the current ring and reschedules ALARM_SNOOZE_MINUTES from
 * now (wrapping past midnight correctly). No-op if not ringing. */
void alarm_snooze (AlarmState *alarm, const ClockTime *now);

/* Silences the current ring without rescheduling -- the alarm will
 * ring again at its normal time (tomorrow, if today's has passed).
 * No-op if not ringing. */
void alarm_dismiss (AlarmState *alarm);

#endif /* FLIPCLOCK_ALARM_H */
