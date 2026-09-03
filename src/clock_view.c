#include "clock_view.h"

#include "clock_model.h"
#include "flip_anim.h"
#include "radio.h"
#include "alarm.h"
#include "buzzer.h"

#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define COL_CASE_BODY      0.541, 0.353, 0.204   /* #8a5a34 */
#define COL_CASE_TOP       0.588, 0.388, 0.235   /* #96633c */
#define COL_CASE_BORDER    0.361, 0.231, 0.129   /* #5c3b21 */
#define COL_KNOB_FILL      0.420, 0.271, 0.153   /* #6b4527 */
#define COL_KNOB_BORDER    0.247, 0.165, 0.090   /* #3f2a17 */
#define COL_PANEL_BG       0.086, 0.075, 0.067   /* #161311 */
#define COL_PANEL_TRIM     0.910, 0.890, 0.847   /* #e8e2d8 */
#define COL_DIGIT_BG       0.047, 0.039, 0.035   /* #0c0a09 */
#define COL_DIGIT_TEXT     0.949, 0.929, 0.886   /* #f2ede2 */
#define COL_AMPM_TEXT      0.788, 0.761, 0.706   /* #c9c2b4 */
#define COL_SCALE_TEXT     0.812, 0.788, 0.741   /* #cfc9bd */
#define COL_SCALE_TICK     0.290, 0.271, 0.235   /* #4a453c */
#define COL_NEEDLE         0.847, 0.294, 0.165   /* #d84b2a */

typedef struct {
  FlipUnit hour_unit;
  FlipUnit tens_min_unit;
  FlipUnit ones_min_unit;
  gint64 last_frame_time_us;

  RadioController radio;
  double needle_dial_x;
  gboolean dragging;

  AlarmState alarm;
  Buzzer buzzer;
  gboolean was_ringing;

  double volume;
  gboolean dragging_volume;
  double volume_drag_start_y;
  double volume_drag_start_value;
} ClockViewState;

#define FLIP_ANIM_DURATION_SECONDS 0.45

#define DIGIT_CELL_Y 198
#define DIGIT_CELL_H 56
#define HOUR_CELL_X 132
#define HOUR_CELL_W 48
#define COLON_DOT_X 188
#define TENMIN_CELL_X 196
#define TENMIN_CELL_W 32
#define ONEMIN_CELL_X 230
#define ONEMIN_CELL_W 32
#define DIGIT_FONT_SIZE 34

#define NEEDLE_PIVOT_X 458
#define NEEDLE_PIVOT_Y 280
#define NEEDLE_TIP_Y   200
#define NEEDLE_MIN_X   303
#define NEEDLE_MAX_X   578
#define TUNER_HIT_X0   270
#define TUNER_HIT_X1   590
#define TUNER_HIT_Y0   200
#define TUNER_HIT_Y1   245

#define ALARM_HOUR_KNOB_X 185
#define ALARM_HOUR_KNOB_Y 155
#define ALARM_HOUR_KNOB_R 10
#define ALARM_MIN_KNOB_X  216
#define ALARM_MIN_KNOB_Y  155
#define ALARM_MIN_KNOB_R  10
#define SNOOZE_KNOB_X     450
#define SNOOZE_KNOB_Y     155
#define SNOOZE_KNOB_R     9
#define ALARM_SWITCH_X    480
#define ALARM_SWITCH_Y    150
#define ALARM_SWITCH_W    24
#define ALARM_SWITCH_H    10
#define VOLUME_KNOB_X     595
#define VOLUME_KNOB_Y     155
#define VOLUME_KNOB_R     10
#define VOLUME_DRAG_RANGE_PX 120.0

static void
rounded_rect (cairo_t *cr, double x, double y, double w, double h, double r)
{
  cairo_new_sub_path (cr);
  cairo_arc (cr, x + w - r, y + r, r, -M_PI / 2, 0);
  cairo_arc (cr, x + w - r, y + h - r, r, 0, M_PI / 2);
  cairo_arc (cr, x + r, y + h - r, r, M_PI / 2, M_PI);
  cairo_arc (cr, x + r, y + r, r, M_PI, 3 * M_PI / 2);
  cairo_close_path (cr);
}

static void
draw_knob (cairo_t *cr, double cx, double cy, double r)
{
  cairo_set_source_rgb (cr, COL_KNOB_FILL);
  cairo_arc (cr, cx, cy, r, 0, 2 * M_PI);
  cairo_fill_preserve (cr);
  cairo_set_source_rgb (cr, COL_KNOB_BORDER);
  cairo_set_line_width (cr, 2.0);
  cairo_stroke (cr);
}

static void
draw_text (cairo_t *cr, double x, double y, const char *text,
           double size, gboolean bold, gboolean italic)
{
  cairo_select_font_face (cr, "Georgia",
                          italic ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL,
                          bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, size);
  cairo_move_to (cr, x, y);
  cairo_show_text (cr, text);
}

static void
draw_digit_centered (cairo_t *cr, double cx, double cy, const char *label, double size)
{
  cairo_select_font_face (cr, "Georgia", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size (cr, size);

  cairo_text_extents_t ext;
  cairo_text_extents (cr, label, &ext);

  double x = cx - ext.width / 2.0 - ext.x_bearing;
  double y = cy - ext.height / 2.0 - ext.y_bearing;

  cairo_move_to (cr, x, y);
  cairo_show_text (cr, label);
}

static void
draw_half_digit (cairo_t *cr, double cell_cx, double cell_x, double cell_w,
                  double hinge_y, double half_h, gboolean top_half,
                  const char *label, double scale_factor)
{
  double clip_y = top_half ? hinge_y - half_h : hinge_y;
  double safe_scale = scale_factor > 0.001 ? scale_factor : 0.001;

  cairo_save (cr);

  cairo_translate (cr, 0, hinge_y);
  cairo_scale (cr, 1.0, safe_scale);
  cairo_translate (cr, 0, -hinge_y);

  cairo_rectangle (cr, cell_x, clip_y, cell_w, half_h);
  cairo_clip (cr);

  cairo_set_source_rgb (cr, COL_DIGIT_TEXT);
  draw_digit_centered (cr, cell_cx, hinge_y, label, DIGIT_FONT_SIZE);

  cairo_restore (cr);
}

static void
draw_flip_cell (cairo_t *cr, double cell_x, double cell_w,
                 double hinge_y, double half_h, const FlipUnit *unit)
{
  double cell_cx = cell_x + cell_w / 2.0;

  if (!unit->animating)
    {
      draw_half_digit (cr, cell_cx, cell_x, cell_w, hinge_y, half_h, TRUE, unit->current_label, 1.0);
      draw_half_digit (cr, cell_cx, cell_x, cell_w, hinge_y, half_h, FALSE, unit->current_label, 1.0);
    }
  else if (unit->progress < 0.5)
    {
      double angle = unit->progress * M_PI;
      double s = cos (angle);

      draw_half_digit (cr, cell_cx, cell_x, cell_w, hinge_y, half_h, TRUE, unit->target_label, 1.0);
      draw_half_digit (cr, cell_cx, cell_x, cell_w, hinge_y, half_h, TRUE, unit->current_label, s);
      draw_half_digit (cr, cell_cx, cell_x, cell_w, hinge_y, half_h, FALSE, unit->current_label, 1.0);
    }
  else
    {
      double local_progress = (unit->progress - 0.5) * 2.0;
      double s = sin (local_progress * M_PI / 2.0);

      draw_half_digit (cr, cell_cx, cell_x, cell_w, hinge_y, half_h, TRUE, unit->target_label, 1.0);
      draw_half_digit (cr, cell_cx, cell_x, cell_w, hinge_y, half_h, FALSE, unit->current_label, 1.0);
      draw_half_digit (cr, cell_cx, cell_x, cell_w, hinge_y, half_h, FALSE, unit->target_label, s);
    }

  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_set_line_width (cr, 1.0);
  cairo_move_to (cr, cell_x, hinge_y);
  cairo_line_to (cr, cell_x + cell_w, hinge_y);
  cairo_stroke (cr);
}

static void
tick_frame (GtkWidget *widget, ClockViewState *state)
{
  GdkFrameClock *frame_clock = gtk_widget_get_frame_clock (widget);
  gint64 frame_time_us = frame_clock ? gdk_frame_clock_get_frame_time (frame_clock) : 0;

  double delta_seconds = 0.0;
  if (state->last_frame_time_us != 0 && frame_time_us > state->last_frame_time_us)
    delta_seconds = (frame_time_us - state->last_frame_time_us) / 1000000.0;
  state->last_frame_time_us = frame_time_us;

  ClockTime ct;
  clock_time_now (&ct);

  int minute_tens = ct.minute / 10;
  if (minute_tens < 0) minute_tens = 0;
  if (minute_tens > 5) minute_tens = 5;

  char hour_buf[FLIP_LABEL_MAX];
  char tens_buf[FLIP_LABEL_MAX];
  char ones_buf[FLIP_LABEL_MAX];
  snprintf (hour_buf, sizeof (hour_buf), "%d", ct.display_hour);
  snprintf (tens_buf, sizeof (tens_buf), "%d", minute_tens);
  snprintf (ones_buf, sizeof (ones_buf), "%d", ct.minute % 10);

  flip_unit_set_target (&state->hour_unit, hour_buf);
  flip_unit_set_target (&state->tens_min_unit, tens_buf);
  flip_unit_set_target (&state->ones_min_unit, ones_buf);

  flip_unit_advance (&state->hour_unit, delta_seconds, FLIP_ANIM_DURATION_SECONDS);
  flip_unit_advance (&state->tens_min_unit, delta_seconds, FLIP_ANIM_DURATION_SECONDS);
  flip_unit_advance (&state->ones_min_unit, delta_seconds, FLIP_ANIM_DURATION_SECONDS);

  alarm_check (&state->alarm, &ct);
  if (state->alarm.ringing && !state->was_ringing)
    buzzer_start (&state->buzzer);
  else if (!state->alarm.ringing && state->was_ringing)
    buzzer_stop (&state->buzzer);
  state->was_ringing = state->alarm.ringing;
}

static gboolean
on_tick (GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data)
{
  (void) frame_clock;
  (void) user_data;

  ClockViewState *state = g_object_get_data (G_OBJECT (widget), "clock-view-state");
  tick_frame (widget, state);
  gtk_widget_queue_draw (widget);

  return G_SOURCE_CONTINUE;
}

static gboolean
point_in_tuner_hitregion (double x, double y)
{
  return x >= TUNER_HIT_X0 && x <= TUNER_HIT_X1
      && y >= TUNER_HIT_Y0 && y <= TUNER_HIT_Y1;
}

static gboolean
point_in_circle (double x, double y, double cx, double cy, double r)
{
  double dx = x - cx;
  double dy = y - cy;
  return (dx * dx + dy * dy) <= (r * r);
}

static gboolean
point_in_rect (double x, double y, double rx, double ry, double rw, double rh)
{
  return x >= rx && x <= rx + rw && y >= ry && y <= ry + rh;
}

static void
on_drag_begin (GtkGestureDrag *gesture, double start_x, double start_y, gpointer user_data)
{
  (void) user_data;
  GtkWidget *widget = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (gesture));
  ClockViewState *state = g_object_get_data (G_OBJECT (widget), "clock-view-state");

  state->dragging = FALSE;
  state->dragging_volume = FALSE;

  if (point_in_tuner_hitregion (start_x, start_y))
    {
      state->dragging = TRUE;
      state->needle_dial_x = CLAMP (start_x, NEEDLE_MIN_X, NEEDLE_MAX_X);
      gtk_widget_queue_draw (widget);
      return;
    }

  if (point_in_circle (start_x, start_y, VOLUME_KNOB_X, VOLUME_KNOB_Y, VOLUME_KNOB_R))
    {
      state->dragging_volume = TRUE;
      state->volume_drag_start_y = start_y;
      state->volume_drag_start_value = state->volume;
      return;
    }
}

static void
on_drag_update (GtkGestureDrag *gesture, double offset_x, double offset_y, gpointer user_data)
{
  (void) user_data;
  GtkWidget *widget = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (gesture));
  ClockViewState *state = g_object_get_data (G_OBJECT (widget), "clock-view-state");

  if (state->dragging)
    {
      double start_x, start_y;
      gtk_gesture_drag_get_start_point (gesture, &start_x, &start_y);
      state->needle_dial_x = CLAMP (start_x + offset_x, NEEDLE_MIN_X, NEEDLE_MAX_X);
      gtk_widget_queue_draw (widget);
      return;
    }

  if (state->dragging_volume)
    {
      double delta = -offset_y / VOLUME_DRAG_RANGE_PX;
      double new_volume = state->volume_drag_start_value + delta;
      if (new_volume < 0.0) new_volume = 0.0;
      if (new_volume > 1.0) new_volume = 1.0;
      state->volume = new_volume;
      radio_controller_set_volume (&state->radio, state->volume);
      gtk_widget_queue_draw (widget);
      return;
    }
}

static void
on_drag_end (GtkGestureDrag *gesture, double offset_x, double offset_y, gpointer user_data)
{
  (void) user_data;
  GtkWidget *widget = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (gesture));
  ClockViewState *state = g_object_get_data (G_OBJECT (widget), "clock-view-state");

  if (state->dragging)
    {
      double start_x, start_y;
      gtk_gesture_drag_get_start_point (gesture, &start_x, &start_y);
      double final_x = CLAMP (start_x + offset_x, NEEDLE_MIN_X, NEEDLE_MAX_X);

      int preset = radio_nearest_preset (final_x);
      state->needle_dial_x = radio_presets[preset].dial_x;
      radio_controller_tune (&state->radio, preset);

      state->dragging = FALSE;
      gtk_widget_queue_draw (widget);
      return;
    }

  if (state->dragging_volume)
    {
      (void) offset_y;
      state->dragging_volume = FALSE;
      gtk_widget_queue_draw (widget);
      return;
    }
}

static void
on_click_pressed (GtkGestureClick *gesture, gint n_press, double x, double y, gpointer user_data)
{
  (void) n_press;
  (void) user_data;
  GtkWidget *widget = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (gesture));
  ClockViewState *state = g_object_get_data (G_OBJECT (widget), "clock-view-state");

  if (point_in_circle (x, y, ALARM_HOUR_KNOB_X, ALARM_HOUR_KNOB_Y, ALARM_HOUR_KNOB_R))
    {
      alarm_adjust_hour (&state->alarm, 1);
      gtk_widget_queue_draw (widget);
      return;
    }

  if (point_in_circle (x, y, ALARM_MIN_KNOB_X, ALARM_MIN_KNOB_Y, ALARM_MIN_KNOB_R))
    {
      alarm_adjust_minute (&state->alarm, 1);
      gtk_widget_queue_draw (widget);
      return;
    }

  if (point_in_circle (x, y, SNOOZE_KNOB_X, SNOOZE_KNOB_Y, SNOOZE_KNOB_R))
    {
      if (state->alarm.ringing)
        {
          ClockTime ct;
          clock_time_now (&ct);
          alarm_snooze (&state->alarm, &ct);
        }
      else
        {
          alarm_toggle_pm (&state->alarm);
        }
      gtk_widget_queue_draw (widget);
      return;
    }

  if (point_in_rect (x, y, ALARM_SWITCH_X, ALARM_SWITCH_Y, ALARM_SWITCH_W, ALARM_SWITCH_H))
    {
      alarm_set_enabled (&state->alarm, !state->alarm.enabled);
      gtk_widget_queue_draw (widget);
      return;
    }
}

static void
draw_func (GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data)
{
  (void) width;
  (void) height;
  (void) user_data;

  ClockViewState *state = g_object_get_data (G_OBJECT (area), "clock-view-state");

  cairo_set_source_rgb (cr, 0.11, 0.10, 0.09);
  rounded_rect (cr, 90, 322, 520, 22, 6);
  cairo_fill (cr);
  cairo_set_source_rgb (cr, 0.165, 0.141, 0.125);
  rounded_rect (cr, 70, 304, 560, 26, 8);
  cairo_fill (cr);

  cairo_set_source_rgb (cr, COL_CASE_BODY);
  rounded_rect (cr, 55, 140, 590, 30, 8);
  cairo_fill (cr);
  cairo_set_source_rgb (cr, 0.247, 0.184, 0.129);
  cairo_rectangle (cr, 55, 146, 590, 2);
  cairo_fill (cr);
  cairo_set_source_rgb (cr, 0.431, 0.333, 0.251);
  cairo_rectangle (cr, 55, 156, 590, 2);
  cairo_fill (cr);
  cairo_set_source_rgb (cr, COL_CASE_BORDER);
  cairo_set_line_width (cr, 2.0);
  rounded_rect (cr, 55, 140, 590, 30, 8);
  cairo_stroke (cr);

  cairo_set_source_rgb (cr, 0.227, 0.173, 0.125);
  for (int x = 240; x <= 432; x += 12)
    {
      cairo_arc (cr, x, 148, 1.6, 0, 2 * M_PI);
      cairo_fill (cr);
    }

  draw_knob (cr, ALARM_HOUR_KNOB_X, ALARM_HOUR_KNOB_Y, ALARM_HOUR_KNOB_R);
  cairo_set_source_rgb (cr, COL_CASE_BORDER);
  cairo_arc (cr, ALARM_HOUR_KNOB_X, ALARM_HOUR_KNOB_Y, 6.5, 0, 2 * M_PI);
  cairo_set_line_width (cr, 0.8);
  cairo_stroke (cr);
  cairo_rectangle (cr, ALARM_HOUR_KNOB_X - 1.5, ALARM_HOUR_KNOB_Y - 6, 3, 4.5);
  cairo_fill (cr);

  draw_knob (cr, ALARM_MIN_KNOB_X, ALARM_MIN_KNOB_Y, ALARM_MIN_KNOB_R);
  cairo_set_source_rgb (cr, COL_CASE_BORDER);
  cairo_arc (cr, ALARM_MIN_KNOB_X, ALARM_MIN_KNOB_Y, 6.5, 0, 2 * M_PI);
  cairo_set_line_width (cr, 0.8);
  cairo_stroke (cr);
  cairo_rectangle (cr, ALARM_MIN_KNOB_X - 1.5, ALARM_MIN_KNOB_Y - 6, 3, 4.5);
  cairo_fill (cr);

  draw_knob (cr, SNOOZE_KNOB_X, SNOOZE_KNOB_Y, SNOOZE_KNOB_R);
  cairo_set_source_rgb (cr, COL_CASE_BORDER);
  cairo_arc (cr, SNOOZE_KNOB_X, SNOOZE_KNOB_Y, 5.8, 0, 2 * M_PI);
  cairo_set_line_width (cr, 0.8);
  cairo_stroke (cr);
  cairo_rectangle (cr, SNOOZE_KNOB_X - 1.5, SNOOZE_KNOB_Y - 5.5, 3, 4);
  cairo_fill (cr);
  if (state->alarm.ringing)
    {
      cairo_set_source_rgba (cr, COL_NEEDLE, 0.6);
      cairo_arc (cr, SNOOZE_KNOB_X, SNOOZE_KNOB_Y, SNOOZE_KNOB_R + 3, 0, 2 * M_PI);
      cairo_set_line_width (cr, 2.0);
      cairo_stroke (cr);
    }

  {
    double switch_fill_r = state->alarm.enabled ? 0.482 : 0.173;
    double switch_fill_g = state->alarm.enabled ? 0.686 : 0.173;
    double switch_fill_b = state->alarm.enabled ? 0.404 : 0.165;
    double toggle_w = ALARM_SWITCH_W / 2.0;
    double toggle_x = state->alarm.enabled
        ? ALARM_SWITCH_X + toggle_w
        : ALARM_SWITCH_X;

    cairo_set_source_rgb (cr, 0.09, 0.09, 0.09);
    rounded_rect (cr, ALARM_SWITCH_X, ALARM_SWITCH_Y, ALARM_SWITCH_W, ALARM_SWITCH_H, 3);
    cairo_fill (cr);

    cairo_set_source_rgb (cr, switch_fill_r, switch_fill_g, switch_fill_b);
    rounded_rect (cr, toggle_x, ALARM_SWITCH_Y, toggle_w, ALARM_SWITCH_H, 3);
    cairo_fill (cr);
  }

  draw_knob (cr, VOLUME_KNOB_X, VOLUME_KNOB_Y, VOLUME_KNOB_R);
  {
    double angle = (-135.0 + state->volume * 270.0) * M_PI / 180.0;
    double ind_x = VOLUME_KNOB_X + (VOLUME_KNOB_R - 4) * sin (angle);
    double ind_y = VOLUME_KNOB_Y - (VOLUME_KNOB_R - 4) * cos (angle);
    cairo_set_source_rgb (cr, COL_CASE_BORDER);
    cairo_set_line_width (cr, 2.5);
    cairo_move_to (cr, VOLUME_KNOB_X, VOLUME_KNOB_Y);
    cairo_line_to (cr, ind_x, ind_y);
    cairo_stroke (cr);
  }

  cairo_set_source_rgb (cr, 0.290, 0.227, 0.161);
  cairo_move_to (cr, 55, 175); cairo_line_to (cr, 30, 210);
  cairo_line_to (cr, 30, 317); cairo_line_to (cr, 55, 285);
  cairo_close_path (cr);
  cairo_fill (cr);
  cairo_move_to (cr, 645, 175); cairo_line_to (cr, 670, 210);
  cairo_line_to (cr, 670, 317); cairo_line_to (cr, 645, 285);
  cairo_close_path (cr);
  cairo_fill (cr);

  cairo_set_source_rgb (cr, 0.039, 0.039, 0.039);
  rounded_rect (cr, 30, 170, 640, 140, 18);
  cairo_fill (cr);
  cairo_set_source_rgb (cr, 0.788, 0.788, 0.788);
  cairo_set_line_width (cr, 3.0);
  rounded_rect (cr, 30, 170, 640, 140, 18);
  cairo_stroke (cr);
  cairo_set_source_rgba (cr, 0.847, 0.847, 0.847, 0.55);
  rounded_rect (cr, 34, 174, 632, 7, 4);
  cairo_fill (cr);

  cairo_set_source_rgb (cr, COL_PANEL_BG);
  rounded_rect (cr, 55, 188, 600, 102, 10);
  cairo_fill (cr);

  ClockTime ct;
  clock_time_now (&ct);
  cairo_set_source_rgb (cr, COL_NEEDLE);
  draw_text (cr, 90, 218, clock_time_ampm_string (&ct), 13, TRUE, FALSE);

  cairo_set_source_rgb (cr, COL_DIGIT_BG);
  rounded_rect (cr, 126, 196, 140, 60, 4);
  cairo_fill (cr);

  double hinge_y = DIGIT_CELL_Y + DIGIT_CELL_H / 2.0;
  double half_h = DIGIT_CELL_H / 2.0;

  draw_flip_cell (cr, HOUR_CELL_X, HOUR_CELL_W, hinge_y, half_h, &state->hour_unit);
  draw_flip_cell (cr, TENMIN_CELL_X, TENMIN_CELL_W, hinge_y, half_h, &state->tens_min_unit);
  draw_flip_cell (cr, ONEMIN_CELL_X, ONEMIN_CELL_W, hinge_y, half_h, &state->ones_min_unit);

  cairo_set_source_rgb (cr, COL_DIGIT_TEXT);
  cairo_arc (cr, COLON_DOT_X, hinge_y - 10, 2.5, 0, 2 * M_PI);
  cairo_fill (cr);
  cairo_arc (cr, COLON_DOT_X, hinge_y + 10, 2.5, 0, 2 * M_PI);
  cairo_fill (cr);

  {
    char alarm_buf[24];
    gint64 now_us = g_get_monotonic_time ();
    gboolean blink_on = ((now_us / 500000) % 2) == 0;

    if (state->alarm.ringing && blink_on)
      {
        cairo_set_source_rgb (cr, COL_NEEDLE);
        draw_text (cr, 128, 282, "ALARM!", 12, TRUE, FALSE);
      }
    else if (state->alarm.enabled)
      {
        snprintf (alarm_buf, sizeof (alarm_buf), "AL %d:%02d%s",
                  state->alarm.hour, state->alarm.minute,
                  state->alarm.is_pm ? "P" : "A");
        cairo_set_source_rgb (cr, COL_SCALE_TEXT);
        draw_text (cr, 128, 282, alarm_buf, 11, FALSE, FALSE);
      }
    else if (!state->alarm.ringing)
      {
        snprintf (alarm_buf, sizeof (alarm_buf), "AL OFF %d:%02d%s",
                  state->alarm.hour, state->alarm.minute,
                  state->alarm.is_pm ? "P" : "A");
        cairo_set_source_rgb (cr, COL_SCALE_TICK);
        draw_text (cr, 128, 282, alarm_buf, 10, FALSE, FALSE);
      }
  }

  cairo_set_source_rgb (cr, COL_SCALE_TEXT);
  draw_text (cr, 278, 212, "FM", 10, FALSE, FALSE);
  draw_text (cr, 278, 234, "AM", 10, FALSE, FALSE);

  const char *fm_marks[] = { "88", "92", "96", "100", "104", "108" };
  double fm_x[] = { 308, 336, 364, 390, 420, 448 };
  for (int i = 0; i < 6; i++)
    draw_text (cr, fm_x[i], 212, fm_marks[i], 10, FALSE, FALSE);
  draw_text (cr, 483, 212, "MHz", 10, FALSE, FALSE);

  const char *am_marks[] = { "53", "65", "80", "100", "130", "160" };
  double am_x[] = { 308, 333, 358, 386, 414, 442 };
  for (int i = 0; i < 6; i++)
    draw_text (cr, am_x[i], 234, am_marks[i], 10, FALSE, FALSE);
  draw_text (cr, 483, 234, "xkkHz", 10, FALSE, FALSE);

  cairo_set_source_rgb (cr, COL_SCALE_TICK);
  cairo_set_line_width (cr, 1.0);
  cairo_move_to (cr, 303, 218); cairo_line_to (cr, 303, 240); cairo_stroke (cr);
  cairo_move_to (cr, 578, 218); cairo_line_to (cr, 578, 240); cairo_stroke (cr);

  cairo_set_source_rgb (cr, COL_NEEDLE);
  cairo_set_line_width (cr, 3.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_move_to (cr, NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y);
  cairo_line_to (cr, state->needle_dial_x, NEEDLE_TIP_Y);
  cairo_stroke (cr);
  cairo_arc (cr, NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y, 4, 0, 2 * M_PI);
  cairo_fill (cr);

  cairo_set_source_rgb (cr, COL_SCALE_TEXT);
  const char *station_text;
  if (state->radio.has_error)
    station_text = "connection error";
  else if (state->radio.current_preset >= 0)
    station_text = radio_presets[state->radio.current_preset].label;
  else
    station_text = "drag dial to tune";
  draw_text (cr, 278, 282, station_text, 11, FALSE, FALSE);

  cairo_set_source_rgb (cr, COL_DIGIT_TEXT);
  draw_text (cr, 453, 270, "Panasonic", 20, FALSE, TRUE);
}

static void
clock_view_state_free (gpointer data)
{
  ClockViewState *state = data;
  radio_controller_dispose (&state->radio);
  buzzer_dispose (&state->buzzer);
  g_free (state);
}

GtkWidget *
clock_view_new (void)
{
  ClockViewState *state = g_new0 (ClockViewState, 1);

  ClockTime ct;
  clock_time_now (&ct);

  char buf[FLIP_LABEL_MAX];
  snprintf (buf, sizeof (buf), "%d", ct.display_hour);
  flip_unit_init (&state->hour_unit, buf);

  int minute_tens = ct.minute / 10;
  if (minute_tens < 0) minute_tens = 0;
  if (minute_tens > 5) minute_tens = 5;
  snprintf (buf, sizeof (buf), "%d", minute_tens);
  flip_unit_init (&state->tens_min_unit, buf);
  snprintf (buf, sizeof (buf), "%d", ct.minute % 10);
  flip_unit_init (&state->ones_min_unit, buf);

  state->last_frame_time_us = 0;

  radio_controller_init (&state->radio);
  state->needle_dial_x = (NEEDLE_MIN_X + NEEDLE_MAX_X) / 2.0;
  state->dragging = FALSE;

  alarm_init (&state->alarm);
  buzzer_init (&state->buzzer);
  state->was_ringing = FALSE;

  state->volume = 0.7;
  state->dragging_volume = FALSE;
  radio_controller_set_volume (&state->radio, state->volume);

  GtkWidget *area = gtk_drawing_area_new ();
  gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (area), CLOCK_VIEW_WIDTH);
  gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (area), CLOCK_VIEW_HEIGHT);

  g_object_set_data_full (G_OBJECT (area), "clock-view-state", state, clock_view_state_free);

  gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (area), draw_func, NULL, NULL);
  gtk_widget_add_tick_callback (area, on_tick, NULL, NULL);

  GtkGesture *drag = gtk_gesture_drag_new ();
  g_signal_connect (drag, "drag-begin", G_CALLBACK (on_drag_begin), NULL);
  g_signal_connect (drag, "drag-update", G_CALLBACK (on_drag_update), NULL);
  g_signal_connect (drag, "drag-end", G_CALLBACK (on_drag_end), NULL);
  gtk_widget_add_controller (area, GTK_EVENT_CONTROLLER (drag));

  GtkGesture *click = gtk_gesture_click_new ();
  g_signal_connect (click, "pressed", G_CALLBACK (on_click_pressed), NULL);
  gtk_widget_add_controller (area, GTK_EVENT_CONTROLLER (click));

  return area;
}
