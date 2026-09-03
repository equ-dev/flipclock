#include "clock_view.h"

#include "clock_model.h"
#include "flip_anim.h"

#include <math.h>
#include <stdio.h>

/* -std=c11 (strict ISO C) hides POSIX/BSD math macros like M_PI unless
 * a feature-test macro is defined before including <math.h>. Fall back
 * to a literal definition rather than relying on that. */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Colors lifted from the design mockup (see conversation history). */
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

/* Per-view animation state. Phase 3 assumes a single clock-view
 * instance (this app has exactly one window/clock). It's attached to
 * the widget via g_object_set_data_full, so it's still per-widget in
 * principle -- a multi-instance app would work, each ticking
 * independently -- but nothing here is a global singleton. */
typedef struct {
  FlipUnit hour_unit;
  FlipUnit tens_min_unit;
  FlipUnit ones_min_unit;
  gint64 last_frame_time_us; /* 0 until the first tick */
} ClockViewState;

/* How long a single digit's flip takes, start to settle. Real
 * mechanical flip clocks land somewhere around 0.3-0.6s per flip. */
#define FLIP_ANIM_DURATION_SECONDS 0.45

/* Digit cell layout, inside the black display panel (185,192,180,96). */
#define DIGIT_CELL_Y 196
#define DIGIT_CELL_H 88
#define HOUR_CELL_X 190
#define HOUR_CELL_W 62
#define COLON_CELL_X 254
#define TENMIN_CELL_X 268
#define TENMIN_CELL_W 46
#define ONEMIN_CELL_X 316
#define ONEMIN_CELL_W 46
#define DIGIT_FONT_SIZE 48

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

/* Draws label so its glyph bounding box is vertically and horizontally
 * centered at (cx, cy). Using this same anchor for every half-digit
 * draw call is what keeps the top and bottom halves lining up exactly
 * at the seam regardless of which digit is being drawn. */
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

/* Draws one half (top or bottom) of a digit, folded toward the hinge
 * line by scale_factor (1.0 = flat/fully open, ~0.0 = collapsed/
 * edge-on). This is a 2D approximation of the mechanical leaf's
 * rotation: Cairo has no true 3D perspective, so vertical scaling about
 * the hinge stands in for the foreshortening a real rotating card would
 * show. It's the same trick most software flip-clock implementations
 * use, and reads convincingly at this size. */
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

/* Renders one independently-flipping digit cell. When settled, this is
 * just the current digit split by a seam line (matching the real
 * device's look even at rest). While animating, it runs the two-phase
 * leaf sequence: first the old top folds away to reveal the new top
 * (already fixed behind it), then the new bottom unfolds down over the
 * old bottom. */
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
      /* Phase A (progress 0 -> 0.5, leaf angle 0deg -> 90deg): old top
       * leaf collapses toward the hinge; the new top is already drawn
       * underneath, revealed as the leaf shrinks. Bottom is untouched. */
      double angle = unit->progress * M_PI; /* 0 -> pi/2 over this phase */
      double s = cos (angle);

      draw_half_digit (cr, cell_cx, cell_x, cell_w, hinge_y, half_h, TRUE, unit->target_label, 1.0);
      draw_half_digit (cr, cell_cx, cell_x, cell_w, hinge_y, half_h, TRUE, unit->current_label, s);
      draw_half_digit (cr, cell_cx, cell_x, cell_w, hinge_y, half_h, FALSE, unit->current_label, 1.0);
    }
  else
    {
      /* Phase B (progress 0.5 -> 1.0, leaf angle 90deg -> 0deg): new
       * bottom leaf unfolds from the hinge, covering the old bottom
       * that's still drawn underneath. Top is now fully the new digit. */
      double local_progress = (unit->progress - 0.5) * 2.0; /* 0..1 */
      double s = sin (local_progress * M_PI / 2.0);

      draw_half_digit (cr, cell_cx, cell_x, cell_w, hinge_y, half_h, TRUE, unit->target_label, 1.0);
      draw_half_digit (cr, cell_cx, cell_x, cell_w, hinge_y, half_h, FALSE, unit->current_label, 1.0);
      draw_half_digit (cr, cell_cx, cell_x, cell_w, hinge_y, half_h, FALSE, unit->target_label, s);
    }

  /* Seam line across the hinge, matching the real device's split-flap
   * card gap. */
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

static void
draw_func (GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data)
{
  (void) width;
  (void) height;
  (void) user_data;

  ClockViewState *state = g_object_get_data (G_OBJECT (area), "clock-view-state");

  /* Wood case body. */
  cairo_set_source_rgb (cr, COL_CASE_BODY);
  rounded_rect (cr, 40, 60, 620, 300, 14);
  cairo_fill (cr);

  /* Lighter top strip. */
  cairo_save (cr);
  rounded_rect (cr, 40, 60, 620, 300, 14);
  cairo_clip (cr);
  cairo_set_source_rgb (cr, COL_CASE_TOP);
  cairo_rectangle (cr, 40, 60, 620, 60);
  cairo_fill (cr);
  cairo_restore (cr);

  cairo_set_source_rgb (cr, COL_CASE_BORDER);
  cairo_set_line_width (cr, 2.0);
  rounded_rect (cr, 40, 60, 620, 300, 14);
  cairo_stroke (cr);

  /* Top control knobs. */
  draw_knob (cr, 150, 90, 16);
  cairo_set_source_rgb (cr, COL_CASE_BORDER);
  cairo_arc (cr, 150, 90, 4, 0, 2 * M_PI);
  cairo_fill (cr);

  draw_knob (cr, 220, 90, 16);
  cairo_set_source_rgb (cr, COL_CASE_BORDER);
  cairo_arc (cr, 220, 90, 4, 0, 2 * M_PI);
  cairo_fill (cr);

  draw_knob (cr, 470, 90, 14);

  cairo_set_source_rgb (cr, 0.173, 0.173, 0.165); /* #2c2c2a */
  rounded_rect (cr, 540, 82, 26, 12, 3);
  cairo_fill (cr);

  /* Display panel: black oval with white trim. */
  cairo_set_source_rgb (cr, COL_PANEL_BG);
  rounded_rect (cr, 70, 150, 560, 180, 90);
  cairo_fill (cr);
  cairo_set_source_rgb (cr, COL_PANEL_TRIM);
  cairo_set_line_width (cr, 4.0);
  rounded_rect (cr, 70, 150, 560, 180, 90);
  cairo_stroke (cr);

  /* Left-side volume/alarm knob set into the panel. */
  draw_knob (cr, 115, 240, 18);

  /* Digit panel background. */
  cairo_set_source_rgb (cr, COL_DIGIT_BG);
  rounded_rect (cr, 185, 192, 180, 96, 6);
  cairo_fill (cr);

  double hinge_y = DIGIT_CELL_Y + DIGIT_CELL_H / 2.0;
  double half_h = DIGIT_CELL_H / 2.0;

  draw_flip_cell (cr, HOUR_CELL_X, HOUR_CELL_W, hinge_y, half_h, &state->hour_unit);
  draw_flip_cell (cr, TENMIN_CELL_X, TENMIN_CELL_W, hinge_y, half_h, &state->tens_min_unit);
  draw_flip_cell (cr, ONEMIN_CELL_X, ONEMIN_CELL_W, hinge_y, half_h, &state->ones_min_unit);

  /* Colon between hour and minutes -- static, doesn't flip. */
  cairo_set_source_rgb (cr, COL_DIGIT_TEXT);
  cairo_arc (cr, COLON_CELL_X + 6, hinge_y - 14, 3.5, 0, 2 * M_PI);
  cairo_fill (cr);
  cairo_arc (cr, COLON_CELL_X + 6, hinge_y + 14, 3.5, 0, 2 * M_PI);
  cairo_fill (cr);

  /* AM/PM -- reflects the live clock, doesn't flip. */
  ClockTime ct;
  clock_time_now (&ct);
  cairo_set_source_rgb (cr, COL_AMPM_TEXT);
  draw_text (cr, 195, 285, clock_time_ampm_string (&ct), 13, FALSE, FALSE);

  /* Tuner scale labels. Spacing here is wider than the original mockup
   * sketch -- Cairo's default (non-Pango) text layout has no kerning
   * awareness of neighboring calls, so tight label spacing overlapped. */
  cairo_set_source_rgb (cr, COL_SCALE_TEXT);
  draw_text (cr, 395, 205, "FM", 10, FALSE, FALSE);
  draw_text (cr, 395, 278, "AM", 10, FALSE, FALSE);

  const char *fm_marks[] = { "88", "92", "96", "100", "104", "108" };
  double fm_x[] = { 440, 472, 504, 536, 568, 598 };
  for (int i = 0; i < 6; i++)
    draw_text (cr, fm_x[i], 205, fm_marks[i], 10, FALSE, FALSE);
  draw_text (cr, 612, 205, "MHz", 10, FALSE, FALSE);

  const char *am_marks[] = { "53", "60", "70", "90", "100", "130", "160" };
  double am_x[] = { 438, 465, 492, 519, 546, 573, 596 };
  for (int i = 0; i < 7; i++)
    draw_text (cr, am_x[i], 278, am_marks[i], 10, FALSE, FALSE);
  draw_text (cr, 612, 278, "xkkHz", 10, FALSE, FALSE);

  cairo_set_source_rgb (cr, COL_SCALE_TICK);
  cairo_set_line_width (cr, 1.0);
  cairo_move_to (cr, 432, 212); cairo_line_to (cr, 432, 270); cairo_stroke (cr);
  cairo_move_to (cr, 605, 212); cairo_line_to (cr, 605, 270); cairo_stroke (cr);

  /* Tuning needle (Phase 2: fixed placeholder position; Phase 4 will
   * make this reflect the actual tuned frequency). */
  cairo_set_source_rgb (cr, COL_NEEDLE);
  cairo_set_line_width (cr, 3.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_move_to (cr, 500, 290);
  cairo_line_to (cr, 465, 215);
  cairo_stroke (cr);
  cairo_arc (cr, 500, 290, 4, 0, 2 * M_PI);
  cairo_fill (cr);

  /* Wordmark. Bold only, no italic: with Georgia unavailable this
   * substitutes to DejaVu Serif, whose bold-italic kerning at this size
   * rendered "Panasonic" illegibly (looked like "Panasomc"). */
  cairo_set_source_rgb (cr, COL_DIGIT_TEXT);
  draw_text (cr, 480, 322, "Panasonic", 22, TRUE, FALSE);
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

  GtkWidget *area = gtk_drawing_area_new ();
  gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (area), CLOCK_VIEW_WIDTH);
  gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (area), CLOCK_VIEW_HEIGHT);

  g_object_set_data_full (G_OBJECT (area), "clock-view-state", state, g_free);

  gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (area), draw_func, NULL, NULL);
  gtk_widget_add_tick_callback (area, on_tick, NULL, NULL);

  return area;
}
