#include <check.h>
#include <stdlib.h>

#include "../src/alarm.h"

static ClockTime
make_time (int display_hour, int minute, int is_pm)
{
  ClockTime ct;
  ct.display_hour = display_hour;
  ct.minute = minute;
  ct.second = 0;
  ct.is_pm = is_pm;
  return ct;
}

START_TEST (test_init_defaults)
{
  AlarmState alarm;
  alarm_init (&alarm);

  ck_assert_int_eq (alarm.hour, 7);
  ck_assert_int_eq (alarm.minute, 0);
  ck_assert (!alarm.is_pm);
  ck_assert (!alarm.enabled);
  ck_assert (!alarm.ringing);
}
END_TEST

START_TEST (test_adjust_hour_wraps_up)
{
  AlarmState alarm;
  alarm_init (&alarm);
  alarm.hour = 12;
  alarm_adjust_hour (&alarm, 1);
  ck_assert_int_eq (alarm.hour, 1);
}
END_TEST

START_TEST (test_adjust_hour_wraps_down)
{
  AlarmState alarm;
  alarm_init (&alarm);
  alarm.hour = 1;
  alarm_adjust_hour (&alarm, -1);
  ck_assert_int_eq (alarm.hour, 12);
}
END_TEST

START_TEST (test_adjust_minute_wraps_up)
{
  AlarmState alarm;
  alarm_init (&alarm);
  alarm.minute = 59;
  alarm_adjust_minute (&alarm, 1);
  ck_assert_int_eq (alarm.minute, 0);
}
END_TEST

START_TEST (test_adjust_minute_wraps_down)
{
  AlarmState alarm;
  alarm_init (&alarm);
  alarm.minute = 0;
  alarm_adjust_minute (&alarm, -1);
  ck_assert_int_eq (alarm.minute, 59);
}
END_TEST

START_TEST (test_disabled_alarm_never_rings)
{
  AlarmState alarm;
  alarm_init (&alarm);
  alarm.hour = 7;
  alarm.minute = 0;
  alarm.is_pm = false;
  alarm.enabled = false;

  ClockTime now = make_time (7, 0, 0);
  alarm_check (&alarm, &now);

  ck_assert (!alarm.ringing);
}
END_TEST

START_TEST (test_enabled_alarm_rings_at_matching_time)
{
  AlarmState alarm;
  alarm_init (&alarm);
  alarm.hour = 7;
  alarm.minute = 30;
  alarm.is_pm = false;
  alarm.enabled = true;

  ClockTime before = make_time (7, 29, 0);
  alarm_check (&alarm, &before);
  ck_assert (!alarm.ringing);

  ClockTime at_time = make_time (7, 30, 0);
  alarm_check (&alarm, &at_time);
  ck_assert (alarm.ringing);
}
END_TEST

START_TEST (test_does_not_retrigger_within_same_minute)
{
  AlarmState alarm;
  alarm_init (&alarm);
  alarm.hour = 7;
  alarm.minute = 30;
  alarm.is_pm = false;
  alarm.enabled = true;

  ClockTime at_time = make_time (7, 30, 0);
  alarm_check (&alarm, &at_time);
  ck_assert (alarm.ringing);

  alarm_dismiss (&alarm);
  ck_assert (!alarm.ringing);

  /* Still the same minute -- should NOT re-ring after dismiss. */
  alarm_check (&alarm, &at_time);
  ck_assert (!alarm.ringing);
}
END_TEST

START_TEST (test_rings_again_next_day_same_time)
{
  /* This is the case the edge-triggered design exists for: a naive
   * "remember the total-minutes value that last triggered" approach
   * would permanently block the alarm once that minute-of-day value
   * recurs, since it wraps every 1440 minutes (i.e. every day). */
  AlarmState alarm;
  alarm_init (&alarm);
  alarm.hour = 7;
  alarm.minute = 30;
  alarm.is_pm = false;
  alarm.enabled = true;

  ClockTime at_time = make_time (7, 30, 0);
  alarm_check (&alarm, &at_time);
  ck_assert (alarm.ringing);
  alarm_dismiss (&alarm);

  /* Time moves away from the target minute (simulating the rest of
   * the day passing) ... */
  ClockTime later = make_time (7, 31, 0);
  alarm_check (&alarm, &later);
  ck_assert (!alarm.ringing);

  /* ... and comes back to 7:30 AM the "next day". */
  alarm_check (&alarm, &at_time);
  ck_assert (alarm.ringing);
}
END_TEST

START_TEST (test_snooze_silences_and_reschedules)
{
  AlarmState alarm;
  alarm_init (&alarm);
  alarm.hour = 7;
  alarm.minute = 30;
  alarm.is_pm = false;
  alarm.enabled = true;

  ClockTime at_time = make_time (7, 30, 0);
  alarm_check (&alarm, &at_time);
  ck_assert (alarm.ringing);

  alarm_snooze (&alarm, &at_time);
  ck_assert (!alarm.ringing);
  ck_assert (alarm.snoozed);

  /* Not yet time -- shouldn't re-ring early. */
  ClockTime before_snooze_end = make_time (7, 34, 0);
  alarm_check (&alarm, &before_snooze_end);
  ck_assert (!alarm.ringing);

  /* ALARM_SNOOZE_MINUTES (5) later -- should ring again. */
  ClockTime snooze_end = make_time (7, 35, 0);
  alarm_check (&alarm, &snooze_end);
  ck_assert (alarm.ringing);
}
END_TEST

START_TEST (test_snooze_wraps_past_midnight)
{
  AlarmState alarm;
  alarm_init (&alarm);
  alarm.hour = 11;
  alarm.minute = 58;
  alarm.is_pm = true; /* 11:58 PM */
  alarm.enabled = true;

  ClockTime at_time = make_time (11, 58, 1); /* is_pm = 1 */
  alarm_check (&alarm, &at_time);
  ck_assert (alarm.ringing);

  alarm_snooze (&alarm, &at_time);
  ck_assert (alarm.snoozed);

  /* 11:58 PM + 5 minutes = 12:03 AM. */
  ClockTime after_midnight = make_time (12, 3, 0); /* is_pm = 0 (AM) */
  alarm_check (&alarm, &after_midnight);
  ck_assert (alarm.ringing);
}
END_TEST

START_TEST (test_snooze_noop_when_not_ringing)
{
  AlarmState alarm;
  alarm_init (&alarm);
  ClockTime now = make_time (7, 30, 0);
  alarm_snooze (&alarm, &now); /* not ringing -- should be a no-op */

  ck_assert (!alarm.snoozed);
}
END_TEST

START_TEST (test_disabling_while_ringing_dismisses)
{
  AlarmState alarm;
  alarm_init (&alarm);
  alarm.hour = 7;
  alarm.minute = 30;
  alarm.is_pm = false;
  alarm.enabled = true;

  ClockTime at_time = make_time (7, 30, 0);
  alarm_check (&alarm, &at_time);
  ck_assert (alarm.ringing);

  alarm_set_enabled (&alarm, false);
  ck_assert (!alarm.ringing);
}
END_TEST

static Suite *
alarm_suite (void)
{
  Suite *s = suite_create ("Alarm");
  TCase *tc = tcase_create ("Core");

  tcase_add_test (tc, test_init_defaults);
  tcase_add_test (tc, test_adjust_hour_wraps_up);
  tcase_add_test (tc, test_adjust_hour_wraps_down);
  tcase_add_test (tc, test_adjust_minute_wraps_up);
  tcase_add_test (tc, test_adjust_minute_wraps_down);
  tcase_add_test (tc, test_disabled_alarm_never_rings);
  tcase_add_test (tc, test_enabled_alarm_rings_at_matching_time);
  tcase_add_test (tc, test_does_not_retrigger_within_same_minute);
  tcase_add_test (tc, test_rings_again_next_day_same_time);
  tcase_add_test (tc, test_snooze_silences_and_reschedules);
  tcase_add_test (tc, test_snooze_wraps_past_midnight);
  tcase_add_test (tc, test_snooze_noop_when_not_ringing);
  tcase_add_test (tc, test_disabling_while_ringing_dismisses);

  suite_add_tcase (s, tc);
  return s;
}

int
main (void)
{
  Suite *s = alarm_suite ();
  SRunner *sr = srunner_create (s);

  srunner_run_all (sr, CK_NORMAL);
  int failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
