#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "../src/flip_anim.h"

START_TEST (test_init_is_settled)
{
  FlipUnit unit;
  flip_unit_init (&unit, "3");

  ck_assert_str_eq (unit.current_label, "3");
  ck_assert (!unit.animating);
  ck_assert_double_eq_tol (unit.progress, 0.0, 1e-9);
}
END_TEST

START_TEST (test_set_target_starts_animation)
{
  FlipUnit unit;
  flip_unit_init (&unit, "3");
  flip_unit_set_target (&unit, "4");

  ck_assert (unit.animating);
  ck_assert_str_eq (unit.target_label, "4");
  ck_assert_str_eq (unit.current_label, "3"); /* not settled yet */
}
END_TEST

START_TEST (test_set_target_same_value_is_noop)
{
  FlipUnit unit;
  flip_unit_init (&unit, "5");
  flip_unit_set_target (&unit, "5");

  ck_assert (!unit.animating);
}
END_TEST

START_TEST (test_set_target_ignored_while_mid_flip)
{
  FlipUnit unit;
  flip_unit_init (&unit, "1");
  flip_unit_set_target (&unit, "2");
  flip_unit_advance (&unit, 0.1, 0.5); /* now partway through */

  flip_unit_set_target (&unit, "9"); /* should be ignored */

  ck_assert_str_eq (unit.target_label, "2");
}
END_TEST

START_TEST (test_advance_partial_keeps_animating)
{
  FlipUnit unit;
  flip_unit_init (&unit, "0");
  flip_unit_set_target (&unit, "1");
  flip_unit_advance (&unit, 0.25, 0.5); /* half of a 0.5s duration */

  ck_assert (unit.animating);
  ck_assert_double_eq_tol (unit.progress, 0.5, 1e-9);
  ck_assert_str_eq (unit.current_label, "0"); /* not settled yet */
}
END_TEST

START_TEST (test_advance_completes_and_settles)
{
  FlipUnit unit;
  flip_unit_init (&unit, "8");
  flip_unit_set_target (&unit, "9");
  flip_unit_advance (&unit, 0.5, 0.5); /* exactly the full duration */

  ck_assert (!unit.animating);
  ck_assert_str_eq (unit.current_label, "9");
  ck_assert_double_eq_tol (unit.progress, 0.0, 1e-9);
}
END_TEST

START_TEST (test_advance_overshoot_still_settles_cleanly)
{
  FlipUnit unit;
  flip_unit_init (&unit, "1");
  flip_unit_set_target (&unit, "2");
  flip_unit_advance (&unit, 10.0, 0.5); /* way more than needed */

  ck_assert (!unit.animating);
  ck_assert_str_eq (unit.current_label, "2");
}
END_TEST

START_TEST (test_advance_while_settled_is_noop)
{
  FlipUnit unit;
  flip_unit_init (&unit, "6");
  flip_unit_advance (&unit, 1.0, 0.5); /* no target set, nothing to do */

  ck_assert (!unit.animating);
  ck_assert_str_eq (unit.current_label, "6");
}
END_TEST

START_TEST (test_two_digit_hour_label)
{
  FlipUnit unit;
  flip_unit_init (&unit, "11");
  flip_unit_set_target (&unit, "12");
  flip_unit_advance (&unit, 0.5, 0.5);

  ck_assert_str_eq (unit.current_label, "12");
}
END_TEST

static Suite *
flip_anim_suite (void)
{
  Suite *s = suite_create ("FlipAnim");
  TCase *tc = tcase_create ("Core");

  tcase_add_test (tc, test_init_is_settled);
  tcase_add_test (tc, test_set_target_starts_animation);
  tcase_add_test (tc, test_set_target_same_value_is_noop);
  tcase_add_test (tc, test_set_target_ignored_while_mid_flip);
  tcase_add_test (tc, test_advance_partial_keeps_animating);
  tcase_add_test (tc, test_advance_completes_and_settles);
  tcase_add_test (tc, test_advance_overshoot_still_settles_cleanly);
  tcase_add_test (tc, test_advance_while_settled_is_noop);
  tcase_add_test (tc, test_two_digit_hour_label);

  suite_add_tcase (s, tc);
  return s;
}

int
main (void)
{
  Suite *s = flip_anim_suite ();
  SRunner *sr = srunner_create (s);

  srunner_run_all (sr, CK_NORMAL);
  int failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
