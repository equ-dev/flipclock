#include <gtk/gtk.h>
#include <gst/gst.h>

#include "clock_view.h"

/* Fixed window size matched to the flip clock's proportions (see design
 * mockup: a wide, short case housing the digit panel and tuner dial). */
#define FLIPCLOCK_WIN_WIDTH  CLOCK_VIEW_WIDTH
#define FLIPCLOCK_WIN_HEIGHT CLOCK_VIEW_HEIGHT

static void
activate (GtkApplication *app, gpointer user_data)
{
  (void) user_data;

  GtkWidget *window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Flip Clock");
  gtk_window_set_default_size (GTK_WINDOW (window),
                                FLIPCLOCK_WIN_WIDTH,
                                FLIPCLOCK_WIN_HEIGHT);
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);

  /* Phase 2: static chrome from the design mockup (wood case, digit
   * panel, tuner scale, knobs, wordmark). Not yet wired to real
   * clock/tuner data or the flip animation -- that's Phases 3-4. */
  GtkWidget *view = clock_view_new ();
  gtk_window_set_child (GTK_WINDOW (window), view);

  gtk_window_present (GTK_WINDOW (window));
}

int
main (int argc, char **argv)
{
  gst_init (&argc, &argv);

  GtkApplication *app = gtk_application_new ("com.example.flipclock",
                                              G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  int status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);
  return status;
}
