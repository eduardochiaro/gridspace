#include "animations.h"
#include "animations/sideload.h"
#include "animations/random.h"
#include "animations/matrix.h"
#include <stdlib.h>

// Random seed for animation
static uint32_t s_random_seed = 0;

// Simple pseudo-random number generator
static uint32_t random_next(void) {
  s_random_seed = (s_random_seed * 1103515245 + 12345) & 0x7fffffff;
  return s_random_seed;
}

static void animation_timer_callback(void *data);

void animations_init(AnimationState *state) {
  state->type = ANIM_NONE;
  state->progress = 0.0f;
  state->fade = 1.0f;
  state->active = false;
  state->timer = NULL;
  state->layer = NULL;
  
  // Seed random with current time
  time_t t = time(NULL);
  s_random_seed = (uint32_t)t;
}

void animations_start_load(AnimationState *state, AnimationType type) {
  if (state->timer) {
    app_timer_cancel(state->timer);
  }
  
  state->type = type;
  state->progress = 0.0f;
  state->fade = 1.0f;
  state->active = true;
  
  // Reseed random for new animation
  time_t t = time(NULL);
  s_random_seed = (uint32_t)t + (random_next() & 0xFF);
  
  // Start animation timer (30 FPS)
  state->timer = app_timer_register(33, animation_timer_callback, state);
}

void animations_stop(AnimationState *state) {
  if (state->timer) {
    app_timer_cancel(state->timer);
    state->timer = NULL;
  }
  state->active = false;
}

void animations_update(AnimationState *state) {
  if (!state->active) return;
  
  switch (state->type) {
    case ANIM_WAVE_FILL:
    case ANIM_RANDOM_POP:
    case ANIM_MATRIX:
      // All animations use same timing: 0.0 to 0.7 (animation), 0.7 to 1.0 (fade out)
      state->progress += 0.02f;  // ~1.5 seconds total
      
      if (state->progress >= 0.7f) {
        // Start fade out
        state->fade = 1.0f - ((state->progress - 0.7f) / 0.3f);
        if (state->fade < 0.0f) state->fade = 0.0f;
      }
      
      if (state->progress >= 1.0f) {
        animations_stop(state);
      }
      break;
      
    default:
      animations_stop(state);
      break;
  }
}

static void animation_timer_callback(void *data) {
  AnimationState *state = (AnimationState *)data;
  animations_update(state);
  
  // Trigger redraw
  if (state->layer) {
    layer_mark_dirty(state->layer);
  }
  
  if (state->active) {
    // Schedule next frame
    state->timer = app_timer_register(33, animation_timer_callback, state);
  }
}

// Draw wave fill animation
static void draw_wave_fill(GContext *ctx, AnimationState *state,
                          int grid_cols, int grid_rows,
                          int grid_offset_x, int grid_offset_y,
                          int cell_size, int full_size, int full_offset,
                          int partial_size, int partial_offset,
                          GColor fg_color, GColor secondary_color) {
  // Use sideload animation
  draw_sideload_animation(ctx, state->progress, state->fade,
                         grid_cols, grid_rows,
                         grid_offset_x, grid_offset_y,
                         cell_size, full_size, full_offset,
                         partial_size, partial_offset,
                         fg_color, secondary_color);
}

void animations_draw(GContext *ctx, AnimationState *state,
                    int grid_cols, int grid_rows,
                    int grid_offset_x, int grid_offset_y,
                    int cell_size, int full_size, int full_offset,
                    int partial_size, int partial_offset,
                    GColor fg_color, GColor secondary_color) {
  if (!state->active) return;
  
  switch (state->type) {
    case ANIM_WAVE_FILL:
      draw_wave_fill(ctx, state, grid_cols, grid_rows,
                    grid_offset_x, grid_offset_y,
                    cell_size, full_size, full_offset,
                    partial_size, partial_offset,
                    fg_color, secondary_color);
      break;
    
    case ANIM_RANDOM_POP:
      draw_random_animation(ctx, state->progress, state->fade,
                           grid_cols, grid_rows,
                           grid_offset_x, grid_offset_y,
                           cell_size, full_size, full_offset,
                           partial_size, partial_offset,
                           fg_color, secondary_color);
      break;
    
    case ANIM_MATRIX:
      draw_matrix_animation(ctx, state->progress, state->fade,
                           grid_cols, grid_rows,
                           grid_offset_x, grid_offset_y,
                           cell_size, full_size, full_offset,
                           partial_size, partial_offset,
                           fg_color, secondary_color);
      break;
      
    default:
      break;
  }
}

bool animations_is_active(AnimationState *state) {
  return state->active;
}
