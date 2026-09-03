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

#define RADIO_MAX_RETRIES 3
#define RADIO_RETRY_DELAY_SECONDS 3

typedef struct {
  GstElement *playbin;
  int current_preset;   /* -1 = nothing tuned/playing */
  gboolean has_error;
  char last_error[256];
  int retry_count;
  guint retry_source_id; /* 0 = none scheduled */
} RadioController;

/* Pure decision of whether another reconnect attempt should be made.
 * Separated out so the retry-capping logic is unit-testable without
 * GStreamer or a real (or fake) network involved. */
gboolean radio_should_retry (int retry_count);

/* Creates the playbin pipeline. Must be called after gst_init(). */
void radio_controller_init (RadioController *radio);

/* Tunes to radio_presets[preset_index] and starts playback. */
void radio_controller_tune (RadioController *radio, int preset_index);

/* Sets playback volume, 0.0 (silent) to 1.0 (full). Safe to call at
 * any time, tuned or not -- takes effect immediately if playing, and
 * whenever playback next starts otherwise. */
void radio_controller_set_volume (RadioController *radio, double volume);

/* Stops playback without tearing down the pipeline. */
void radio_controller_stop (RadioController *radio);

/* Tears down the pipeline. Safe to call on an already-disposed or
 * never-initialized controller. */
void radio_controller_dispose (RadioController *radio);

#endif /* FLIPCLOCK_RADIO_H */
