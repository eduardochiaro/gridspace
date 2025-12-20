#include "random.h"
#include <stdlib.h>

// Random seed for animation
static uint32_t s_random_seed = 0;

// Simple pseudo-random number generator
static uint32_t random_next(void) {
  s_random_seed = (s_random_seed * 1103515245 + 12345) & 0x7fffffff;
  return s_random_seed;
}

// Initialize seed
void random_init_seed(uint32_t seed) {
  s_random_seed = seed;
}

// Draw random pop animation
// Cells appear randomly, turn full -> partial -> disappear
void draw_random_animation(GContext *ctx, float progress, float fade,
                           int grid_cols, int grid_rows,
                           int grid_offset_x, int grid_offset_y,
                           int cell_size, int full_size, int full_offset,
                           int partial_size, int partial_offset,
                           GColor fg_color, GColor secondary_color) {
  // Animation phases over 1.5 seconds:
  // 0.0 - 0.7: Cells appear and transition (full -> partial -> disappear)
  // 0.7 - 1.0: Fade out
  
  uint32_t saved_seed = s_random_seed;
  
  for (int r = 0; r < grid_rows; r++) {
    for (int c = 0; c < grid_cols; c++) {
      // Generate deterministic random value for this cell
      s_random_seed = (uint32_t)((r * 2654435761U) ^ (c * 2246822519U) ^ saved_seed);
      uint32_t rand_val = random_next();
      uint32_t rand_val2 = random_next();
      
      // Each cell has a random start time (0.0 to 0.5)
      float cell_start = (rand_val % 500) / 1000.0f;
      
      // Cell duration: 0.5 seconds (full -> partial -> disappear)
      float cell_duration = 0.5f;
      float cell_progress = (progress - cell_start) / cell_duration;
      
      // Skip if cell hasn't started yet or already finished
      if (cell_progress < 0.0f || cell_progress > 1.0f) continue;
      
      // Apply fade
      if (fade < 0.3f) continue;
      
      // Determine cell state based on progress
      // 0.0 - 0.33: Full
      // 0.33 - 0.66: Partial
      // 0.66 - 1.0: Disappear
      int cell_state;
      if (cell_progress < 0.33f) {
        cell_state = 2; // CELL_FULL
      } else if (cell_progress < 0.66f) {
        cell_state = 1; // CELL_PARTIAL
      } else {
        continue; // Disappeared
      }
      
      // Determine color (55% foreground, 45% secondary)
      bool use_secondary = (rand_val2 % 100) < 45;
      
      // Set color
      GColor color = use_secondary ? secondary_color : fg_color;
      graphics_context_set_fill_color(ctx, color);
      
      int x = grid_offset_x + c * cell_size;
      int y = grid_offset_y + r * cell_size;
      
      if (cell_state == 2) {
        // CELL_FULL
        graphics_fill_rect(ctx, GRect(x + full_offset, y + full_offset, full_size, full_size), 0, GCornerNone);
      } else {
        // CELL_PARTIAL
        graphics_fill_rect(ctx, GRect(x + partial_offset, y + partial_offset, partial_size, partial_size), 0, GCornerNone);
      }
    }
  }
  
  s_random_seed = saved_seed;
}
