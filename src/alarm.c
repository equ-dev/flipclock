#include "alarm.h"

#include <string.h>

static int
total_minutes (int display_hour, int minute, bool is_pm)
{
  int hour24 = display_hour % 12; /* 12 -> 0 */
  if (is_pm)
    hour24 += 12;
  return hour24 * 60 + minute;
}

void
alarm_init (AlarmState *alarm)
{
  memset (alarm, 0, sizeof (*alarm));
  alarm->hour = 7;
  alarm->minute = 0;
  alarm->is_pm = false; /* 7:00 AM default */
  alarm->enabled = false;
}

void
alarm_adjust_hour (AlarmState *alarm, int delta)
{
  int h = alarm->hour + delta;
  h = ((h - 1) % 12 + 12) % 12 + 1; /* wrap into 1-12 */
  alarm->hour = h;
}

void
alarm_adjust_minute (AlarmState *alarm, int delta)
{
  int m = (alarm->minute + delta) % 60;
  if (m < 0)
    m += 60;
  alarm->minute = m;
}

void
alarm_toggle_pm (AlarmState *alarm)
{
  alarm->is_pm = !alarm->is_pm;
}

void
alarm_set_enabled (AlarmState *alarm, bool enabled)
{
  alarm->enabled = enabled;
  if (!enabled && alarm->ringing)
    alarm_dismiss (alarm);
}

void
alarm_check (AlarmState *alarm, const ClockTime *now)
{
  int now_total = total_minutes (now->display_hour, now->minute, now->is_pm);

  if (alarm->snoozed)
    {
      if (now_total == alarm->snoozed_until_total_minutes)
        {
          alarm->snoozed = false;
          alarm->ringing = true;
        }
      return;
    }

  if (!alarm->enabled)
    {
      alarm->was_matching_last_check = false;
      return;
    }

  int target_total = total_minutes (alarm->hour, alarm->minute, alarm->is_pm);
  bool matching_now = (now_total == target_total);

  if (matching_now && !alarm->was_matching_last_check)
    alarm->ringing = true;

  alarm->was_matching_last_check = matching_now;
}

void
alarm_snooze (AlarmState *alarm, const ClockTime *now)
{
  if (!alarm->ringing)
    return;

  int now_total = total_minutes (now->display_hour, now->minute, now->is_pm);

  alarm->ringing = false;
  alarm->snoozed = true;
  alarm->snoozed_until_total_minutes = (now_total + ALARM_SNOOZE_MINUTES) % 1440;
}

void
alarm_dismiss (AlarmState *alarm)
{
  alarm->ringing = false;
}
