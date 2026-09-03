#include "radio.h"

#include <math.h>
#include <string.h>

/* Dial_x values match the FM scale marks already drawn in clock_view.c
 * (92, 100, 104 MHz), so dragging the needle toward one of those marks
 * visually and functionally lines up with the preset that plays. */
const RadioPreset radio_presets[RADIO_PRESET_COUNT] = {
  { 472.0, "92.1 Groove Salad",  "https://ice5.somafm.com/groovesalad-128-mp3" },
  { 536.0, "100.5 Drone Zone",   "https://ice5.somafm.com/dronezone-128-mp3" },
  { 568.0, "104.7 Deep Space",   "https://ice5.somafm.com/deepspaceone-128-mp3" },
};

int
radio_nearest_preset (double x)
{
  int best_index = 0;
  double best_distance = fabs (x - radio_presets[0].dial_x);

  for (int i = 1; i < RADIO_PRESET_COUNT; i++)
    {
      double distance = fabs (x - radio_presets[i].dial_x);
      if (distance < best_distance)
        {
          best_distance = distance;
          best_index = i;
        }
    }

  return best_index;
}

gboolean
radio_should_retry (int retry_count)
{
  return retry_count < RADIO_MAX_RETRIES;
}

static void apply_uri_and_play (RadioController *radio, int preset_index);

static gboolean
on_retry_timeout (gpointer user_data)
{
  RadioController *radio = user_data;
  radio->retry_source_id = 0;
  apply_uri_and_play (radio, radio->current_preset);
  return G_SOURCE_REMOVE;
}

static gboolean
on_bus_message (GstBus *bus, GstMessage *message, gpointer user_data)
{
  (void) bus;
  RadioController *radio = user_data;

  switch (GST_MESSAGE_TYPE (message))
    {
    case GST_MESSAGE_ERROR:
      {
        GError *err = NULL;
        gchar *debug = NULL;
        gst_message_parse_error (message, &err, &debug);

        radio->has_error = TRUE;
        g_snprintf (radio->last_error, sizeof (radio->last_error),
                    "%s", err ? err->message : "unknown error");

        g_printerr ("flipclock radio error: %s (%s)\n",
                    err ? err->message : "unknown", debug ? debug : "");

        g_clear_error (&err);
        g_free (debug);

        if (radio->current_preset >= 0 && radio_should_retry (radio->retry_count)
            && radio->retry_source_id == 0)
          {
            radio->retry_count++;
            radio->retry_source_id =
                g_timeout_add_seconds (RADIO_RETRY_DELAY_SECONDS, on_retry_timeout, radio);
          }
        break;
      }
    case GST_MESSAGE_EOS:
      /* Live streams don't normally EOS; treat it like a dropped
       * connection so the UI can reflect that something stopped. */
      radio->has_error = TRUE;
      g_snprintf (radio->last_error, sizeof (radio->last_error), "%s",
                  "stream ended unexpectedly");
      break;
    default:
      break;
    }

  return TRUE; /* keep watching */
}

void
radio_controller_init (RadioController *radio)
{
  memset (radio, 0, sizeof (*radio));
  radio->current_preset = -1;

  radio->playbin = gst_element_factory_make ("playbin", "flipclock-player");

  if (radio->playbin != NULL)
    {
      GstBus *bus = gst_element_get_bus (radio->playbin);
      gst_bus_add_watch (bus, on_bus_message, radio);
      gst_object_unref (bus);
    }
}

static void
apply_uri_and_play (RadioController *radio, int preset_index)
{
  if (radio->playbin == NULL)
    return;
  if (preset_index < 0 || preset_index >= RADIO_PRESET_COUNT)
    return;

  radio->has_error = FALSE;
  radio->last_error[0] = '\0';

  gst_element_set_state (radio->playbin, GST_STATE_NULL);
  g_object_set (radio->playbin, "uri", radio_presets[preset_index].stream_url, NULL);
  gst_element_set_state (radio->playbin, GST_STATE_PLAYING);

  radio->current_preset = preset_index;
}

void
radio_controller_tune (RadioController *radio, int preset_index)
{
  if (radio->playbin == NULL)
    return;
  if (preset_index < 0 || preset_index >= RADIO_PRESET_COUNT)
    return;

  radio->retry_count = 0; /* fresh user-initiated tune, not a retry */

  if (radio->retry_source_id != 0)
    {
      g_source_remove (radio->retry_source_id);
      radio->retry_source_id = 0;
    }

  apply_uri_and_play (radio, preset_index);
}

void
radio_controller_set_volume (RadioController *radio, double volume)
{
  if (radio->playbin == NULL)
    return;

  double clamped = volume;
  if (clamped < 0.0) clamped = 0.0;
  if (clamped > 1.0) clamped = 1.0;

  g_object_set (radio->playbin, "volume", clamped, NULL);
}

void
radio_controller_stop (RadioController *radio)
{
  if (radio->playbin == NULL)
    return;

  if (radio->retry_source_id != 0)
    {
      g_source_remove (radio->retry_source_id);
      radio->retry_source_id = 0;
    }

  gst_element_set_state (radio->playbin, GST_STATE_NULL);
  radio->current_preset = -1;
}

void
radio_controller_dispose (RadioController *radio)
{
  if (radio->playbin == NULL)
    return;

  if (radio->retry_source_id != 0)
    {
      g_source_remove (radio->retry_source_id);
      radio->retry_source_id = 0;
    }

  gst_element_set_state (radio->playbin, GST_STATE_NULL);
  gst_object_unref (radio->playbin);
  radio->playbin = NULL;
}
