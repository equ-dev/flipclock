#include "flip_anim.h"

#include <string.h>

void
flip_unit_init (FlipUnit *unit, const char *initial_label)
{
  memset (unit, 0, sizeof (*unit));
  strncpy (unit->current_label, initial_label, FLIP_LABEL_MAX - 1);
  strncpy (unit->target_label, initial_label, FLIP_LABEL_MAX - 1);
  unit->progress = 0.0;
  unit->animating = false;
}

void
flip_unit_set_target (FlipUnit *unit, const char *new_label)
{
  if (unit->animating)
    return; /* let the in-flight flip finish first */

  if (strncmp (unit->current_label, new_label, FLIP_LABEL_MAX) == 0)
    return; /* already showing this value, nothing to do */

  strncpy (unit->target_label, new_label, FLIP_LABEL_MAX - 1);
  unit->target_label[FLIP_LABEL_MAX - 1] = '\0';
  unit->progress = 0.0;
  unit->animating = true;
}

void
flip_unit_advance (FlipUnit *unit, double delta_seconds, double duration_seconds)
{
  if (!unit->animating)
    return;

  unit->progress += delta_seconds / duration_seconds;

  if (unit->progress >= 1.0)
    {
      strncpy (unit->current_label, unit->target_label, FLIP_LABEL_MAX);
      unit->animating = false;
      unit->progress = 0.0;
    }
}
