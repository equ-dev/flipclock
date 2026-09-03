#include "buzzer.h"

#include <string.h>

#define BUZZER_BEEP_INTERVAL_MS 400

static guint buzzer_beep_source_id = 0; /* single alarm instance in this app */
static gboolean buzzer_beep_on = FALSE;

static gboolean
on_beep_toggle (gpointer user_data)
{
  Buzzer *buzzer = user_data;
  if (buzzer->pipeline == NULL)
    return G_SOURCE_REMOVE;

  buzzer_beep_on = !buzzer_beep_on;

  GstElement *vol = gst_bin_get_by_name (GST_BIN (buzzer->pipeline), "vol");
  if (vol != NULL)
    {
      g_object_set (vol, "volume", buzzer_beep_on ? 0.6 : 0.0, NULL);
      gst_object_unref (vol);
    }

  return G_SOURCE_CONTINUE;
}

void
buzzer_init (Buzzer *buzzer)
{
  memset (buzzer, 0, sizeof (*buzzer));

  GError *error = NULL;
  buzzer->pipeline = gst_parse_launch (
      "audiotestsrc freq=880 wave=square ! audioconvert "
      "! volume name=vol volume=0.0 ! autoaudiosink",
      &error);

  if (error != NULL)
    {
      g_printerr ("flipclock buzzer: failed to build pipeline: %s\n", error->message);
      g_clear_error (&error);
      buzzer->pipeline = NULL;
    }
}

void
buzzer_start (Buzzer *buzzer)
{
  if (buzzer->pipeline == NULL)
    return;

  buzzer_beep_on = FALSE;
  gst_element_set_state (buzzer->pipeline, GST_STATE_PLAYING);

  if (buzzer_beep_source_id == 0)
    buzzer_beep_source_id = g_timeout_add (BUZZER_BEEP_INTERVAL_MS, on_beep_toggle, buzzer);
}

void
buzzer_stop (Buzzer *buzzer)
{
  if (buzzer->pipeline == NULL)
    return;

  if (buzzer_beep_source_id != 0)
    {
      g_source_remove (buzzer_beep_source_id);
      buzzer_beep_source_id = 0;
    }

  gst_element_set_state (buzzer->pipeline, GST_STATE_NULL);
}

void
buzzer_dispose (Buzzer *buzzer)
{
  if (buzzer->pipeline == NULL)
    return;

  buzzer_stop (buzzer);
  gst_object_unref (buzzer->pipeline);
  buzzer->pipeline = NULL;
}
