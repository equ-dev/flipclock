#ifndef FLIPCLOCK_CLOCK_VIEW_H
#define FLIPCLOCK_CLOCK_VIEW_H

#include <gtk/gtk.h>

#define CLOCK_VIEW_WIDTH  700
#define CLOCK_VIEW_HEIGHT 390

/* Creates the drawing area that renders the flip-clock chrome (wood
 * case, digit panel, tuner scale, knobs, wordmark) per the design
 * mockup. Phase 2: static placeholder content only, no animation and
 * no live clock/tuner data wired in yet. */
GtkWidget *clock_view_new (void);

#endif /* FLIPCLOCK_CLOCK_VIEW_H */
