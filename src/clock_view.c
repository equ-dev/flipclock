#include "clock_view.h"

#include <math.h>

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
draw_func (GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data)
{
  (void) area;
  (void) width;
  (void) height;
  (void) user_data;

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

  /* Digit panel (Phase 2: static placeholder time, not yet wired to
   * clock_model -- that lands in Phase 3). */
  cairo_set_source_rgb (cr, COL_DIGIT_BG);
  rounded_rect (cr, 185, 192, 180, 96, 6);
  cairo_fill (cr);
  cairo_set_source_rgb (cr, COL_DIGIT_TEXT);
  draw_text (cr, 205, 262, "2:34", 56, TRUE, FALSE);
  cairo_set_source_rgb (cr, COL_AMPM_TEXT);
  draw_text (cr, 195, 285, "PM", 13, FALSE, FALSE);

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
  GtkWidget *area = gtk_drawing_area_new ();
  gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (area), CLOCK_VIEW_WIDTH);
  gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (area), CLOCK_VIEW_HEIGHT);
  gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (area), draw_func, NULL, NULL);
  return area;
}
