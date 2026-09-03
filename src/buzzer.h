#ifndef FLIPCLOCK_BUZZER_H
#define FLIPCLOCK_BUZZER_H

#include <gst/gst.h>

/*
 * The alarm buzzer is a separate GStreamer pipeline from the radio
 * tuner's playbin, built from audiotestsrc (a locally-generated tone,
 * no network involved). This is deliberate: the alarm must work even
 * if the internet radio stream is down or unreachable.
 */
typedef struct {
  GstElement *pipeline;
} Buzzer;

/* Builds the tone pipeline. Must be called after gst_init(). */
void buzzer_init (Buzzer *buzzer);

/* Starts the (looping) alarm tone. */
void buzzer_start (Buzzer *buzzer);

/* Silences the tone without tearing down the pipeline. */
void buzzer_stop (Buzzer *buzzer);

/* Tears down the pipeline. Safe on an already-disposed/never-initialized buzzer. */
void buzzer_dispose (Buzzer *buzzer);

#endif /* FLIPCLOCK_BUZZER_H */
