#ifndef FLIPCLOCK_RADIO_H
#define FLIPCLOCK_RADIO_H

#include <gst/gst.h>

/*
 * The tuner is a real internet-radio player wearing an FM/AM dial as
 * its skin: turning the dial doesn't tune actual RF frequencies, it
 * snaps to the nearest of a small set of preset internet streams,
 * positioned at specific pixel locations along the drawn frequency
 * scale so they visually line up with FM marks the user can drag to.
 */
typedef struct {
  double dial_x;          /* pixel x on the tuner scale this preset sits at */
  const char *label;       /* short display label, e.g. "92.1 Groove Salad" */
  const char *stream_url;
} RadioPreset;

#define RADIO_PRESET_COUNT 3
extern const RadioPreset radio_presets[RADIO_PRESET_COUNT];

/* Returns the index into radio_presets whose dial_x is closest to x.
 * Pure function -- no GStreamer involved, so it's cheaply unit-tested. */
int radio_nearest_preset (double x);

typedef struct {
  GstElement *playbin;
  int current_preset;   /* -1 = nothing tuned/playing */
  gboolean has_error;
  char last_error[256];
} RadioController;

/* Creates the playbin pipeline. Must be called after gst_init(). */
void radio_controller_init (RadioController *radio);

/* Tunes to radio_presets[preset_index] and starts playback. */
void radio_controller_tune (RadioController *radio, int preset_index);

/* Stops playback without tearing down the pipeline. */
void radio_controller_stop (RadioController *radio);

/* Tears down the pipeline. Safe to call on an already-disposed or
 * never-initialized controller. */
void radio_controller_dispose (RadioController *radio);

#endif /* FLIPCLOCK_RADIO_H */
