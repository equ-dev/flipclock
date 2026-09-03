#include <check.h>
#include <stdlib.h>

#include "../src/radio.h"

START_TEST (test_exact_match_first_preset)
{
  int idx = radio_nearest_preset (radio_presets[0].dial_x);
  ck_assert_int_eq (idx, 0);
}
END_TEST

START_TEST (test_exact_match_last_preset)
{
  int idx = radio_nearest_preset (radio_presets[RADIO_PRESET_COUNT - 1].dial_x);
  ck_assert_int_eq (idx, RADIO_PRESET_COUNT - 1);
}
END_TEST

START_TEST (test_snaps_to_nearer_neighbor)
{
  /* A point just on preset 0's side of the midpoint between presets
   * 0 and 1 should snap to preset 0, not preset 1. */
  double midpoint = (radio_presets[0].dial_x + radio_presets[1].dial_x) / 2.0;
  int idx = radio_nearest_preset (midpoint - 1.0);
  ck_assert_int_eq (idx, 0);
}
END_TEST

START_TEST (test_far_left_snaps_to_first_preset)
{
  int idx = radio_nearest_preset (0.0);
  ck_assert_int_eq (idx, 0);
}
END_TEST

START_TEST (test_far_right_snaps_to_last_preset)
{
  int idx = radio_nearest_preset (100000.0);
  ck_assert_int_eq (idx, RADIO_PRESET_COUNT - 1);
}
END_TEST

START_TEST (test_midpoint_between_presets_picks_a_valid_index)
{
  double midpoint = (radio_presets[0].dial_x + radio_presets[1].dial_x) / 2.0;
  int idx = radio_nearest_preset (midpoint);
  ck_assert (idx == 0 || idx == 1);
}
END_TEST

START_TEST (test_should_retry_under_max)
{
  ck_assert (radio_should_retry (0));
  ck_assert (radio_should_retry (RADIO_MAX_RETRIES - 1));
}
END_TEST

START_TEST (test_should_not_retry_at_or_over_max)
{
  ck_assert (!radio_should_retry (RADIO_MAX_RETRIES));
  ck_assert (!radio_should_retry (RADIO_MAX_RETRIES + 1));
}
END_TEST

static Suite *
radio_suite (void)
{
  Suite *s = suite_create ("Radio");
  TCase *tc = tcase_create ("NearestPreset");

  tcase_add_test (tc, test_exact_match_first_preset);
  tcase_add_test (tc, test_exact_match_last_preset);
  tcase_add_test (tc, test_snaps_to_nearer_neighbor);
  tcase_add_test (tc, test_far_left_snaps_to_first_preset);
  tcase_add_test (tc, test_far_right_snaps_to_last_preset);
  tcase_add_test (tc, test_midpoint_between_presets_picks_a_valid_index);
  tcase_add_test (tc, test_should_retry_under_max);
  tcase_add_test (tc, test_should_not_retry_at_or_over_max);

  suite_add_tcase (s, tc);
  return s;
}

int
main (void)
{
  Suite *s = radio_suite ();
  SRunner *sr = srunner_create (s);

  srunner_run_all (sr, CK_NORMAL);
  int failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
