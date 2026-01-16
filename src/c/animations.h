#pragma once
#include <pebble.h>
#include "common.h"

// Animation types
typedef enum {
  ANIM_NONE = 0,
  ANIM_WAVE_FILL,
  ANIM_RANDOM_POP,
  ANIM_MATRIX
} AnimationType;

// Animation state (optimized - uses fixed-point)
typedef struct {
  int16_t progress;     // Fixed-point 8.8: 0-256 (0.0 to 1.0)
  int16_t fade;         // Fixed-point 8.8: 0-256
  AppTimer *timer;
  Layer *layer;
  uint8_t type;
  uint8_t active;
} AnimationState;

// Initialize animation system
void animations_init(AnimationState *state);

// Start a load animation
void animations_start_load(AnimationState *state, AnimationType type);

// Stop current animation
void animations_stop(AnimationState *state);

// Draw current animation (uses GridParams struct)
void animations_draw(GContext *ctx, AnimationState *state, const GridParams *gp);

// Check if animation is active
static inline bool animations_is_active(AnimationState *state) {
  return state->active;
}
