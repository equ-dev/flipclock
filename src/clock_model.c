#include "clock_model.h"

#include <stdio.h>

void
clock_time_from_tm (const struct tm *tm, ClockTime *out)
{
  int hour24 = tm->tm_hour;

  out->minute = tm->tm_min;
  out->second = tm->tm_sec;
  out->is_pm = (hour24 >= 12) ? 1 : 0;

  int display_hour = hour24 % 12;
  if (display_hour == 0)
    display_hour = 12; /* midnight (0) and noon (12) both show as 12 */

  out->display_hour = display_hour;
}

void
clock_time_now (ClockTime *out)
{
  time_t now = time (NULL);
  struct tm local_tm;
  localtime_r (&now, &local_tm);
  clock_time_from_tm (&local_tm, out);
}

int
clock_time_format_hm (const ClockTime *ct, char *buf, size_t buflen)
{
  int written = snprintf (buf, buflen, "%d:%02d", ct->display_hour, ct->minute);
  if (written < 0 || (size_t) written >= buflen)
    return -1;
  return written;
}

const char *
clock_time_ampm_string (const ClockTime *ct)
{
  return ct->is_pm ? "PM" : "AM";
}
