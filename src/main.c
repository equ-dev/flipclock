#include <gtk/gtk.h>

/* Fixed window size matched to the flip clock's proportions (see design
 * mockup: a wide, short case housing the digit panel and tuner dial). */
#define FLIPCLOCK_WIN_WIDTH  700
#define FLIPCLOCK_WIN_HEIGHT 420

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

  /* Placeholder content for Phase 0. Phase 2 replaces this with the
   * wood-case / digit-panel / tuner drawing area from the design mockup. */
  GtkWidget *label = gtk_label_new ("Flip Clock scaffold — Phase 0");
  gtk_window_set_child (GTK_WINDOW (window), label);

  gtk_window_present (GTK_WINDOW (window));
}

int
main (int argc, char **argv)
{
  GtkApplication *app = gtk_application_new ("com.example.flipclock",
                                              G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  int status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);
  return status;
}
