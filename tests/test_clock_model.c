#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "../src/clock_model.h"

static struct tm
make_tm (int hour, int minute, int second)
{
  struct tm tm;
  memset (&tm, 0, sizeof (tm));
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = second;
  return tm;
}

START_TEST (test_midnight_is_12am)
{
  struct tm tm = make_tm (0, 0, 0);
  ClockTime ct;
  clock_time_from_tm (&tm, &ct);

  ck_assert_int_eq (ct.display_hour, 12);
  ck_assert_int_eq (ct.is_pm, 0);
  ck_assert_str_eq (clock_time_ampm_string (&ct), "AM");
}
END_TEST

START_TEST (test_noon_is_12pm)
{
  struct tm tm = make_tm (12, 0, 0);
  ClockTime ct;
  clock_time_from_tm (&tm, &ct);

  ck_assert_int_eq (ct.display_hour, 12);
  ck_assert_int_eq (ct.is_pm, 1);
  ck_assert_str_eq (clock_time_ampm_string (&ct), "PM");
}
END_TEST

START_TEST (test_one_pm_after_noon)
{
  struct tm tm = make_tm (13, 5, 0);
  ClockTime ct;
  clock_time_from_tm (&tm, &ct);

  ck_assert_int_eq (ct.display_hour, 1);
  ck_assert_int_eq (ct.is_pm, 1);
}
END_TEST

START_TEST (test_eleven_pm_before_midnight)
{
  struct tm tm = make_tm (23, 59, 0);
  ClockTime ct;
  clock_time_from_tm (&tm, &ct);

  ck_assert_int_eq (ct.display_hour, 11);
  ck_assert_int_eq (ct.minute, 59);
  ck_assert_int_eq (ct.is_pm, 1);
}
END_TEST

START_TEST (test_format_matches_panasonic_mockup)
{
  /* The design mockup shows "2:34" -- 2:34 PM, no leading zero on hour,
   * minute zero-padded to two digits. */
  struct tm tm = make_tm (14, 34, 0);
  ClockTime ct;
  clock_time_from_tm (&tm, &ct);

  char buf[16];
  int written = clock_time_format_hm (&ct, buf, sizeof (buf));

  ck_assert_int_gt (written, 0);
  ck_assert_str_eq (buf, "2:34");
  ck_assert_str_eq (clock_time_ampm_string (&ct), "PM");
}
END_TEST

START_TEST (test_format_zero_pads_minute)
{
  struct tm tm = make_tm (9, 5, 0);
  ClockTime ct;
  clock_time_from_tm (&tm, &ct);

  char buf[16];
  clock_time_format_hm (&ct, buf, sizeof (buf));

  ck_assert_str_eq (buf, "9:05");
}
END_TEST

START_TEST (test_format_reports_error_on_small_buffer)
{
  struct tm tm = make_tm (10, 30, 0);
  ClockTime ct;
  clock_time_from_tm (&tm, &ct);

  char buf[2]; /* too small for "10:30" */
  int written = clock_time_format_hm (&ct, buf, sizeof (buf));

  ck_assert_int_eq (written, -1);
}
END_TEST

START_TEST (test_clock_time_now_populates_valid_range)
{
  ClockTime ct;
  clock_time_now (&ct);

  ck_assert_int_ge (ct.display_hour, 1);
  ck_assert_int_le (ct.display_hour, 12);
  ck_assert_int_ge (ct.minute, 0);
  ck_assert_int_le (ct.minute, 59);
  ck_assert (ct.is_pm == 0 || ct.is_pm == 1);
}
END_TEST

static Suite *
clock_model_suite (void)
{
  Suite *s = suite_create ("ClockModel");
  TCase *tc = tcase_create ("Core");

  tcase_add_test (tc, test_midnight_is_12am);
  tcase_add_test (tc, test_noon_is_12pm);
  tcase_add_test (tc, test_one_pm_after_noon);
  tcase_add_test (tc, test_eleven_pm_before_midnight);
  tcase_add_test (tc, test_format_matches_panasonic_mockup);
  tcase_add_test (tc, test_format_zero_pads_minute);
  tcase_add_test (tc, test_format_reports_error_on_small_buffer);
  tcase_add_test (tc, test_clock_time_now_populates_valid_range);

  suite_add_tcase (s, tc);
  return s;
}

int
main (void)
{
  Suite *s = clock_model_suite ();
  SRunner *sr = srunner_create (s);

  srunner_run_all (sr, CK_NORMAL);
  int failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
