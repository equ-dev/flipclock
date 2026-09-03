#ifndef FLIPCLOCK_FLIP_ANIM_H
#define FLIPCLOCK_FLIP_ANIM_H

#include <stdbool.h>

#define FLIP_LABEL_MAX 4

/*
 * FlipUnit models one independent flipping digit column (e.g. the hour,
 * or the tens-of-minutes digit). It holds the currently-settled label
 * and, while mid-flip, the target label and animation progress. This
 * is deliberately pure state/logic with no drawing or timing source of
 * its own, so it can be unit-tested by feeding it synthetic time deltas.
 */
typedef struct {
  char current_label[FLIP_LABEL_MAX];
  char target_label[FLIP_LABEL_MAX];
  double progress;   /* 0.0..1.0 while animating; 0.0 when settled */
  bool animating;
} FlipUnit;

/* Sets the unit to a settled (non-animating) state showing initial_label. */
void flip_unit_init (FlipUnit *unit, const char *initial_label);

/* Requests a flip to new_label. No-op if new_label already equals the
 * unit's current settled label, or if the unit is already mid-flip
 * (the in-flight flip is allowed to finish before a new one starts, to
 * avoid visually jarring re-targeting). */
void flip_unit_set_target (FlipUnit *unit, const char *new_label);

/* Advances the animation by delta_seconds out of duration_seconds
 * total. No-op if the unit isn't animating. When progress reaches 1.0,
 * settles current_label = target_label, clears animating, and resets
 * progress to 0.0. */
void flip_unit_advance (FlipUnit *unit, double delta_seconds, double duration_seconds);

#endif /* FLIPCLOCK_FLIP_ANIM_H */
