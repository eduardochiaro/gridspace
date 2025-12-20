#include "sideload.h"
#include <stdlib.h>

// Random seed for animation
static uint32_t s_random_seed = 0;

// Simple pseudo-random number generator
static uint32_t random_next(void) {
  s_random_seed = (s_random_seed * 1103515245 + 12345) & 0x7fffffff;
  return s_random_seed;
}

// Initialize seed
void sideload_init_seed(uint32_t seed) {
  s_random_seed = seed;
}

// Draw wave fill animation (sideload effect)
void draw_sideload_animation(GContext *ctx, float progress, float fade,
                              int grid_cols, int grid_rows,
                              int grid_offset_x, int grid_offset_y,
                              int cell_size, int full_size, int full_offset,
                              int partial_size, int partial_offset,
                              GColor fg_color, GColor secondary_color) {
  // Wave sweeps from top to bottom
  // At progress 0.0, wave is at top (row -5)
  // At progress 0.7, wave is past bottom
  float wave_row = progress * (grid_rows + 10) / 0.7f - 5.0f;
  
  // Random cell generation - use consistent seed per cell
  uint32_t saved_seed = s_random_seed;
  
  for (int r = 0; r < grid_rows; r++) {
    for (int c = 0; c < grid_cols; c++) {
      // Check if this row has been touched by the wave
      bool touched_by_wave = (r <= wave_row + 5);
      
      if (!touched_by_wave) continue;
      
      // Generate deterministic "random" value for this cell with better mixing
      s_random_seed = (uint32_t)((r * 2654435761U) ^ (c * 2246822519U) ^ saved_seed);
      uint32_t rand_val = random_next();
      uint32_t rand_val2 = random_next();
      
      // Determine cell state with more randomness (15% empty, 35% partial, 50% full)
      int cell_state;
      int state_rand = rand_val % 100;
      if (state_rand < 15) {
        cell_state = 0; // CELL_EMPTY
      } else if (state_rand < 50) {
        cell_state = 1; // CELL_PARTIAL
      } else {
        cell_state = 2; // CELL_FULL
      }
      
      // Skip empty cells
      if (cell_state == 0) continue;
      
      // Apply fade to entire animation
      if (fade < 0.3f) continue;
      
      // Determine color with more variation (55% foreground, 45% secondary)
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
