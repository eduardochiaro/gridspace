#pragma once
#include <pebble.h>

// Animation types
typedef enum {
  ANIM_NONE = 0,
  ANIM_WAVE_FILL,
  ANIM_RANDOM_POP,
  ANIM_MATRIX,
  // Add more animation types here
} AnimationType;

// Animation state
typedef struct {
  AnimationType type;
  float progress;       // 0.0 to 1.0
  float fade;           // 0.0 to 1.0 for fade out
  bool active;
  AppTimer *timer;
  Layer *layer;         // Layer to mark dirty
} AnimationState;

// Initialize animation system
void animations_init(AnimationState *state);

// Start a load animation
void animations_start_load(AnimationState *state, AnimationType type);

// Stop current animation
void animations_stop(AnimationState *state);

// Update animation (call from timer)
void animations_update(AnimationState *state);

// Draw current animation
void animations_draw(GContext *ctx, AnimationState *state, 
                     int grid_cols, int grid_rows, 
                     int grid_offset_x, int grid_offset_y,
                     int cell_size, int full_size, int full_offset,
                     int partial_size, int partial_offset,
                     GColor fg_color, GColor secondary_color);

// Check if animation is active
bool animations_is_active(AnimationState *state);
