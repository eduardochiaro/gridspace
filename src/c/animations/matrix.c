#include "matrix.h"
#include <stdlib.h>

// Random seed for animation
static uint32_t s_random_seed = 0;

// Simple pseudo-random number generator
static uint32_t random_next(void) {
  s_random_seed = (s_random_seed * 1103515245 + 12345) & 0x7fffffff;
  return s_random_seed;
}

// Initialize seed
void matrix_init_seed(uint32_t seed) {
  s_random_seed = seed;
}

// Draw matrix falling animation
// Columns of cells fall down with bright heads and fading trails
void draw_matrix_animation(GContext *ctx, float progress, float fade,
                           int grid_cols, int grid_rows,
                           int grid_offset_x, int grid_offset_y,
                           int cell_size, int full_size, int full_offset,
                           int partial_size, int partial_offset,
                           GColor fg_color, GColor secondary_color) {
  // Animation over 0.0 to 0.7, then fade 0.7 to 1.0
  
  uint32_t saved_seed = s_random_seed;
  
  // Each column has its own falling stream
  for (int c = 0; c < grid_cols; c++) {
    // Generate deterministic random values for this column
    s_random_seed = (uint32_t)(c * 2654435761U + saved_seed);
    uint32_t rand_val = random_next();
    uint32_t rand_val2 = random_next();
    
    // Column start time (stagger between 0.0 and 0.2)
    float col_start = (rand_val % 200) / 1000.0f;
    
    // Column speed (some fall faster than others)
    float col_speed = 0.8f + ((rand_val2 % 40) / 100.0f); // 0.8 to 1.2
    
    // Calculate column progress (0.0 to 0.7 for movement phase)
    float col_progress = (progress - col_start) * col_speed;
    if (col_progress < 0.0f) continue;
    
    // Head position moves from -1 to grid_rows
    // At col_progress = 0.7, head should be at bottom (grid_rows - 1)
    float head_row = (col_progress / 0.7f) * grid_rows - 1.0f;
    
    // Calculate per-column fade
    float col_fade = fade; // Start with global fade
    if (head_row >= grid_rows - 1) {
      // Head has reached bottom, start fading this column
      float time_at_bottom = col_progress - (0.7f * (grid_rows - 1 + 1) / grid_rows);
      float col_fade_progress = time_at_bottom / 0.3f; // Fade over 0.3 seconds
      col_fade = 1.0f - col_fade_progress;
      if (col_fade < 0.0f) col_fade = 0.0f;
      
      // Skip this column if fully faded
      if (col_fade < 0.1f) continue;
      
      // Clamp head to grid bounds
      head_row = grid_rows - 1;
    }
    
    // Draw all cells from top (0) to head position
    for (int r = 0; r < grid_rows; r++) {
      // Only draw cells at or above the head
      if (r > head_row + 0.5f) continue;
      
      // Apply per-column fade
      if (col_fade < 0.3f) continue;
      
      int cell_state;
      bool use_secondary = false;
      
      // Check if this is the head position
      if (r >= head_row - 0.5f && r <= head_row + 0.5f) {
        // The bright head - full cell, foreground color
        cell_state = 2; // CELL_FULL
        use_secondary = false;
      } else {
        // Trail above the head
        int trail_pos = (int)(head_row - r);
        
        // Alternate cell states: full, partial, full, partial...
        if (trail_pos % 2 == 0) {
          cell_state = 2; // CELL_FULL
        } else {
          cell_state = 1; // CELL_PARTIAL
        }
        
        // Alternate colors: secondary, primary, secondary, primary...
        use_secondary = (trail_pos % 2 == 0);
      }
      
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
