#include "animations.h"
#include "common.h"

// Fixed-point animation step: ~0.02 in 8.8 format = 5/256
#define ANIM_FP_STEP 5

// Fixed-point thresholds
#define FP_07 179  // 0.7 * 256
#define FP_03 77   // 0.3 * 256

static void animation_timer_callback(void *data);

// Inline cell drawing to avoid function call overhead
static inline void draw_cell_fast(GContext *ctx, int x, int y, uint8_t state,
                                  const GridParams *gp, bool use_secondary) {
  if (state == CELL_EMPTY) return;
  graphics_context_set_fill_color(ctx, use_secondary ? gp->secondary_color : gp->fg_color);
  if (state == CELL_FULL) {
    graphics_fill_rect(ctx, GRect(x + gp->full_offset, y + gp->full_offset, 
                                   gp->full_size, gp->full_size), 0, GCornerNone);
  } else {
    graphics_fill_rect(ctx, GRect(x + gp->partial_offset, y + gp->partial_offset,
                                   gp->partial_size, gp->partial_size), 0, GCornerNone);
  }
}

void animations_init(AnimationState *state) {
  state->type = ANIM_NONE;
  state->progress = 0;
  state->fade = FP_ONE;
  state->active = 0;
  state->timer = NULL;
  state->layer = NULL;
  prng_seed((uint32_t)time(NULL));
}

void animations_start_load(AnimationState *state, AnimationType type) {
  if (state->timer) {
    app_timer_cancel(state->timer);
  }
  state->type = type;
  state->progress = 0;
  state->fade = FP_ONE;
  state->active = 1;
  prng_seed((uint32_t)time(NULL) + (prng_next() & 0xFF));
  state->timer = app_timer_register(33, animation_timer_callback, state);
}

void animations_stop(AnimationState *state) {
  if (state->timer) {
    app_timer_cancel(state->timer);
    state->timer = NULL;
  }
  state->active = 0;
}

static void animations_update(AnimationState *state) {
  if (!state->active) return;
  
  state->progress += ANIM_FP_STEP;
  
  if (state->progress >= FP_07) {
    // Fade out: fade = 1.0 - (progress - 0.7) / 0.3
    // In fixed-point: fade = 256 - (progress - 179) * 256 / 77
    int16_t over = state->progress - FP_07;
    state->fade = FP_ONE - (over * FP_ONE / FP_03);
    if (state->fade < 0) state->fade = 0;
  }
  
  if (state->progress >= FP_ONE) {
    animations_stop(state);
  }
}

static void animation_timer_callback(void *data) {
  AnimationState *state = (AnimationState *)data;
  animations_update(state);
  
  if (state->layer) {
    layer_mark_dirty(state->layer);
  }
  
  if (state->active) {
    state->timer = app_timer_register(33, animation_timer_callback, state);
  }
}

// Unified animation drawing - all animations combined
static void draw_animation(GContext *ctx, AnimationState *state, const GridParams *gp) {
  if (state->fade < FP_03) return;
  
  uint32_t saved_seed = g_random_seed;
  
  for (int r = 0; r < gp->rows; r++) {
    for (int c = 0; c < gp->cols; c++) {
      // Deterministic random for this cell
      g_random_seed = (uint32_t)((r * 2654435761U) ^ (c * 2246822519U) ^ saved_seed);
      uint32_t rv1 = prng_next();
      uint32_t rv2 = prng_next();
      
      uint8_t cell_state = CELL_EMPTY;
      bool use_secondary = false;
      bool draw = false;
      
      if (state->type == ANIM_WAVE_FILL) {
        // Wave sweeps top to bottom
        int16_t wave_row = (state->progress * (gp->rows + 10) / FP_07) - 5;
        if (r <= wave_row + 5) {
          int sr = rv1 % 100;
          cell_state = (sr < 15) ? CELL_EMPTY : (sr < 50) ? CELL_PARTIAL : CELL_FULL;
          use_secondary = (rv2 % 100) < 45;
          draw = (cell_state != CELL_EMPTY);
        }
      } 
      else if (state->type == ANIM_RANDOM_POP) {
        // Random pop: cells appear at random times
        int16_t cell_start = (rv1 % 500) * FP_ONE / 1000;  // 0 to ~128 (0.0 to 0.5)
        int16_t cell_dur = FP_HALF;  // 0.5 in fixed-point
        int16_t cell_prog = (state->progress - cell_start) * FP_ONE / cell_dur;
        
        if (cell_prog >= 0 && cell_prog < FP_ONE) {
          // 0-0.33: FULL, 0.33-0.66: PARTIAL, 0.66+: gone
          if (cell_prog < 85) {  // ~0.33 * 256
            cell_state = CELL_FULL;
            draw = true;
          } else if (cell_prog < 169) {  // ~0.66 * 256
            cell_state = CELL_PARTIAL;
            draw = true;
          }
          use_secondary = (rv2 % 100) < 45;
        }
      }
      else if (state->type == ANIM_MATRIX) {
        // Matrix: columns fall independently
        g_random_seed = (uint32_t)(c * 2654435761U + saved_seed);
        rv1 = prng_next();
        rv2 = prng_next();
        
        int16_t col_start = (rv1 % 200) * FP_ONE / 1000;  // 0 to ~51
        int16_t col_speed = 205 + (rv2 % 102);  // 0.8 to 1.2 in FP
        int16_t col_prog = ((state->progress - col_start) * col_speed) >> 8;
        
        if (col_prog >= 0) {
          int16_t head_row = (col_prog * gp->rows / FP_07);
          if (r <= head_row) {
            int trail_pos = head_row - r;
            if (trail_pos == 0) {
              cell_state = CELL_FULL;
              use_secondary = false;
            } else {
              cell_state = (trail_pos & 1) ? CELL_PARTIAL : CELL_FULL;
              use_secondary = !(trail_pos & 1);
            }
            draw = true;
          }
        }
      }
      
      if (draw) {
        int x = gp->offset_x + c * gp->cell_size;
        int y = gp->offset_y + r * gp->cell_size;
        draw_cell_fast(ctx, x, y, cell_state, gp, use_secondary);
      }
    }
  }
  
  g_random_seed = saved_seed;
}

void animations_draw(GContext *ctx, AnimationState *state, const GridParams *gp) {
  if (!state->active) return;
  draw_animation(ctx, state, gp);
}
