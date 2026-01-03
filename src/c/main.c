#include <pebble.h>
#include <string.h>
#include "animations.h"

// Grid cell size (larger on Emery)
#ifdef PBL_PLATFORM_EMERY
  #define CELL_SIZE 6
  #define FULL_SIZE 4
  #define FULL_OFFSET 1
  #define PARTIAL_SIZE 2
  #define PARTIAL_OFFSET 2
#else
  #define CELL_SIZE 5
  #define FULL_SIZE 3
  #define FULL_OFFSET 1
  #define PARTIAL_SIZE 1
  #define PARTIAL_OFFSET 2
#endif

// Cell states
#define CELL_EMPTY 0
#define CELL_PARTIAL 1
#define CELL_FULL 2

static Window *s_window;
static Layer *s_canvas_layer;

// Grid dimensions (calculated based on screen size)
static int s_grid_cols;
static int s_grid_rows;
static int s_grid_offset_x;
static int s_grid_offset_y;

// Cached time values (updated once per minute)
static uint8_t s_hour, s_minute, s_day, s_month, s_week, s_weekday, s_year;
static int8_t s_prev_hour = -1, s_prev_minute = -1;

// Animation constants
#define ANIM_STEP 0.1f
#define ANIM_INTERVAL_MS 100
#define NUM_DIGITS 4

// Animation state for 4 time digits (h1, h2, m1, m2)
static int8_t s_anim_old_digits[NUM_DIGITS] = {-1, -1, -1, -1};
static int8_t s_anim_new_digits[NUM_DIGITS] = {-1, -1, -1, -1};
static float s_anim_progress[NUM_DIGITS] = {1.0f, 1.0f, 1.0f, 1.0f};
static AppTimer *s_anim_timer = NULL;

// Load animation
static AnimationState s_load_anim;

// Cached values
static uint16_t s_steps = 0;
static uint8_t s_battery_level = 0;
static uint16_t s_step_goal = 8000;
static uint8_t s_load_animation = 1;

// Packed boolean flags (saves memory)
static struct {
  uint8_t health_available:1;
  uint8_t show_steps:1;
  uint8_t show_battery:1;
  uint8_t show_date:1;
  uint8_t use_24h:1;
  uint8_t date_left:3;   // 0=MonthName, 1=WeekDay, 2=WeekNum, 3=Day, 4=Month, 5=Year
  uint8_t date_right:3;  // 0=MonthName, 1=WeekDay, 2=WeekNum, 3=Day, 4=Month, 5=Year
} s_flags = {
  .health_available = 0,
  .show_steps = 1,
  .show_battery = 1,
  .show_date = 1,
  .use_24h = 1,
  .date_left = 3,   // Day
  .date_right = 4   // Month
};

// Color settings (3 bytes each = 9 bytes total)
static GColor s_bg_color;
static GColor s_fg_color;
static GColor s_secondary_color;

// Persist keys
#define PERSIST_KEY_BG_COLOR 1
#define PERSIST_KEY_FG_COLOR 2
#define PERSIST_KEY_SECONDARY_COLOR 3
#define PERSIST_KEY_STEP_GOAL 4
#define PERSIST_KEY_SHOW_STEPS 5
#define PERSIST_KEY_SHOW_BATTERY 6
#define PERSIST_KEY_SHOW_DATE 7
#define PERSIST_KEY_USE_24H 8
#define PERSIST_KEY_DATE_LEFT 9
#define PERSIST_KEY_DATE_RIGHT 10
#define PERSIST_KEY_LOAD_ANIMATION 11

// Small digit patterns (3x5 for each digit 0-9) - using only full cells
static const uint8_t small_digit_patterns[10][15] = {
  {2,2,2, 2,0,2, 2,0,2, 2,0,2, 2,2,2}, // 0
  {0,2,0, 0,2,0, 0,2,0, 0,2,0, 0,2,0}, // 1
  {2,2,2, 0,0,2, 2,2,2, 2,0,0, 2,2,2}, // 2
  {2,2,2, 0,0,2, 2,2,2, 0,0,2, 2,2,2}, // 3
  {2,0,2, 2,0,2, 2,2,2, 0,0,2, 0,0,2}, // 4
  {2,2,2, 2,0,0, 2,2,2, 0,0,2, 2,2,2}, // 5
  {2,2,2, 2,0,0, 2,2,2, 2,0,2, 2,2,2}, // 6
  {2,2,2, 0,0,2, 0,2,0, 0,2,0, 0,2,0}, // 7
  {2,2,2, 2,0,2, 2,2,2, 2,0,2, 2,2,2}, // 8
  {2,2,2, 2,0,2, 2,2,2, 0,0,2, 2,2,2}  // 9
};

// Small letter patterns (3x5) for weekday and month names
// Letters: M, O, T, U, W, E, H, F, R, S, A, J, P, G, C, N, D, Y, L
static const uint8_t small_letter_patterns[19][15] = {
  {2,0,2, 2,2,2, 2,2,2, 2,0,2, 2,0,2}, // M  0
  {2,2,2, 2,0,2, 2,0,2, 2,0,2, 2,2,2}, // O  1
  {2,2,2, 0,2,0, 0,2,0, 0,2,0, 0,2,0}, // T  2
  {2,0,2, 2,0,2, 2,0,2, 2,0,2, 2,2,2}, // U  3
  {2,0,2, 2,0,2, 2,2,2, 2,2,2, 2,0,2}, // W  4
  {2,2,2, 2,0,0, 2,2,2, 2,0,0, 2,2,2}, // E  5
  {2,0,2, 2,0,2, 2,2,2, 2,0,2, 2,0,2}, // H  6
  {2,2,2, 2,0,0, 2,2,2, 2,0,0, 2,0,0}, // F  7
  {2,2,2, 2,0,2, 2,2,2, 2,2,0, 2,0,2}, // R  8
  {2,2,2, 2,0,0, 2,2,2, 0,0,2, 2,2,2}, // S  9
  {2,2,2, 2,0,2, 2,2,2, 2,0,2, 2,0,2}, // A  10
  {0,0,2, 0,0,2, 0,0,2, 2,0,2, 2,2,2}, // J  11
  {2,2,2, 2,0,2, 2,2,2, 2,0,0, 2,0,0}, // P  12
  {2,2,2, 2,0,0, 2,0,2, 2,0,2, 2,2,2}, // G  13
  {2,2,2, 2,0,0, 2,0,0, 2,0,0, 2,2,2}, // C  14
  {2,0,2, 2,2,2, 2,2,2, 2,0,2, 2,0,2}, // N  15
  {2,2,0, 2,0,2, 2,0,2, 2,0,2, 2,2,0}, // D  16
  {2,0,2, 2,0,2, 0,2,0, 0,2,0, 0,2,0}, // Y  17
  {2,0,0, 2,0,0, 2,0,0, 2,0,0, 2,2,2}  // L  18
};

// Weekday letter indices: [day][letter] where day: 0=Mon, 1=Tue, 2=Wed, 3=Thu, 4=Fri, 5=Sat, 6=Sun
// Letter indices: M=0, O=1, T=2, U=3, W=4, E=5, H=6, F=7, R=8, S=9, A=10
static const uint8_t weekday_letters[7][2] = {
  {0, 1},  // MO - Monday
  {2, 3},  // TU - Tuesday
  {4, 5},  // WE - Wednesday
  {2, 6},  // TH - Thursday
  {7, 8},  // FR - Friday
  {9, 10}, // SA - Saturday
  {9, 3}   // SU - Sunday
};

// Month letter indices: [month][letter] where month: 0=Jan, 1=Feb, ..., 11=Dec
// Using unique 2-letter codes: JA, FE, MR, AP, MY, JN, JL, AU, SE, OC, NO, DE
static const uint8_t month_letters[12][2] = {
  {11, 10}, // JA - January
  {7, 5},   // FE - February
  {0, 8},   // MR - March
  {10, 12}, // AP - April
  {0, 17},  // MY - May
  {11, 15}, // JN - June
  {11, 18}, // JL - July
  {10, 3},  // AU - August
  {9, 5},   // SE - September
  {1, 14},  // OC - October
  {15, 1},  // NO - November
  {16, 5}   // DE - December
};

// Digit patterns (5x7 for each digit 0-9)
static const uint8_t digit_patterns[10][35] = {
  {1,2,2,2,1, 2,0,0,0,2, 2,0,0,0,2, 2,0,0,0,2, 2,0,0,0,2, 2,0,0,0,2, 1,2,2,2,1}, // 0
  {0,0,2,0,0, 1,2,2,0,0, 0,0,2,0,0, 0,0,2,0,0, 0,0,2,0,0, 0,0,2,0,0, 1,2,2,2,1}, // 1
  {1,2,2,2,1, 0,0,0,0,2, 0,0,0,0,2, 1,2,2,2,1, 2,0,0,0,0, 2,0,0,0,0, 1,2,2,2,1}, // 2
  {1,2,2,2,1, 0,0,0,0,2, 0,0,0,0,2, 0,1,2,2,1, 0,0,0,0,2, 0,0,0,0,2, 1,2,2,2,1}, // 3
  {1,0,0,0,1, 2,0,0,0,2, 2,0,0,0,2, 1,2,2,2,2, 0,0,0,0,2, 0,0,0,0,2, 0,0,0,0,1}, // 4
  {1,2,2,2,1, 2,0,0,0,0, 2,0,0,0,0, 1,2,2,2,1, 0,0,0,0,2, 0,0,0,0,2, 1,2,2,2,1}, // 5
  {1,2,2,2,1, 2,0,0,0,0, 2,0,0,0,0, 2,2,2,2,1, 2,0,0,0,2, 2,0,0,0,2, 1,2,2,2,1}, // 6
  {1,2,2,2,1, 0,0,0,0,2, 0,0,0,0,2, 0,0,0,2,0, 0,0,2,0,0, 0,0,2,0,0, 0,0,1,0,0}, // 7
  {1,2,2,2,1, 2,0,0,0,2, 2,0,0,0,2, 1,2,2,2,1, 2,0,0,0,2, 2,0,0,0,2, 1,2,2,2,1}, // 8
  {1,2,2,2,1, 2,0,0,0,2, 2,0,0,0,2, 1,2,2,2,2, 0,0,0,0,2, 0,0,0,0,2, 1,2,2,2,1}  // 9
};

// Draw a cell at pixel coordinates
static inline void draw_cell_at(GContext *ctx, int x, int y, uint8_t state, bool use_secondary) {
  if (state == CELL_EMPTY) return;
  
  graphics_context_set_fill_color(ctx, use_secondary ? s_secondary_color : s_fg_color);
  
  if (state == CELL_FULL) {
    graphics_fill_rect(ctx, GRect(x + FULL_OFFSET, y + FULL_OFFSET, FULL_SIZE, FULL_SIZE), 0, GCornerNone);
  } else { // CELL_PARTIAL
    graphics_fill_rect(ctx, GRect(x + PARTIAL_OFFSET, y + PARTIAL_OFFSET, PARTIAL_SIZE, PARTIAL_SIZE), 0, GCornerNone);
  }
}

// Helper to draw 2x2 checkerboard pattern
static inline void draw_checkerboard_2x2(GContext *ctx, int col, int row, bool inverted, bool use_gray) {
  int x0 = s_grid_offset_x + col * CELL_SIZE;
  int y0 = s_grid_offset_y + row * CELL_SIZE;
  int x1 = s_grid_offset_x + (col + 1) * CELL_SIZE;
  int y1 = s_grid_offset_y + (row + 1) * CELL_SIZE;
  
  if (!inverted) {
    draw_cell_at(ctx, x0, y0, CELL_FULL, use_gray);
    draw_cell_at(ctx, x1, y0, CELL_PARTIAL, use_gray);
    draw_cell_at(ctx, x0, y1, CELL_PARTIAL, use_gray);
    draw_cell_at(ctx, x1, y1, CELL_FULL, use_gray);
  } else {
    draw_cell_at(ctx, x0, y0, CELL_PARTIAL, use_gray);
    draw_cell_at(ctx, x1, y0, CELL_FULL, use_gray);
    draw_cell_at(ctx, x0, y1, CELL_FULL, use_gray);
    draw_cell_at(ctx, x1, y1, CELL_PARTIAL, use_gray);
  }
}

// Draw a large digit directly
static void draw_digit(GContext *ctx, int digit, int col, int row, bool use_gray) {
  if (digit < 0 || digit > 9) return;
  const uint8_t *pattern = digit_patterns[digit];
  
  for (int r = 0; r < 7; r++) {
    for (int c = 0; c < 5; c++) {
      uint8_t state = pattern[r * 5 + c];
      if (state != CELL_EMPTY) {
        int x = s_grid_offset_x + (col + c) * CELL_SIZE;
        int y = s_grid_offset_y + (row + r) * CELL_SIZE;
        draw_cell_at(ctx, x, y, state, use_gray);
      }
    }
  }
}

// Draw animated digit transition (old -> new, top to bottom)
static void draw_digit_animated(GContext *ctx, int old_digit, int new_digit, float progress, int col, int row, bool use_gray) {
  if (old_digit < 0 || old_digit > 9 || new_digit < 0 || new_digit > 9) return;
  
  const uint8_t *old_pattern = digit_patterns[old_digit];
  const uint8_t *new_pattern = digit_patterns[new_digit];
  
  // Progress goes from 0.0 (show old) to 1.0 (show new)
  // Transition line moves from row 0 to row 7
  int transition_row = (int)(progress * 7.0f);
  
  for (int r = 0; r < 7; r++) {
    for (int c = 0; c < 5; c++) {
      uint8_t old_state = old_pattern[r * 5 + c];
      uint8_t new_state = new_pattern[r * 5 + c];
      uint8_t state;
      
      if (r < transition_row) {
        // Above transition line: show new digit
        state = new_state;
      } else if (r == transition_row) {
        // On transition line: show partial/transition state
        if (old_state == CELL_FULL && new_state == CELL_EMPTY) {
          state = CELL_PARTIAL;
        } else if (old_state == CELL_EMPTY && new_state == CELL_FULL) {
          state = CELL_PARTIAL;
        } else if (old_state == CELL_PARTIAL && new_state == CELL_FULL) {
          state = CELL_PARTIAL;
        } else if (old_state == CELL_FULL && new_state == CELL_PARTIAL) {
          state = CELL_PARTIAL;
        } else {
          state = new_state;
        }
      } else {
        // Below transition line: show old digit
        state = old_state;
      }
      
      if (state != CELL_EMPTY) {
        int x = s_grid_offset_x + (col + c) * CELL_SIZE;
        int y = s_grid_offset_y + (row + r) * CELL_SIZE;
        draw_cell_at(ctx, x, y, state, use_gray);
      }
    }
  }
}

// Draw a small digit directly
static void draw_small_digit(GContext *ctx, int digit, int col, int row, bool use_gray) {
  if (digit < 0 || digit > 9) return;
  const uint8_t *pattern = small_digit_patterns[digit];
  
  for (int r = 0; r < 5; r++) {
    for (int c = 0; c < 3; c++) {
      uint8_t state = pattern[r * 3 + c];
      if (state != CELL_EMPTY) {
        int x = s_grid_offset_x + (col + c) * CELL_SIZE;
        int y = s_grid_offset_y + (row + r) * CELL_SIZE;
        draw_cell_at(ctx, x, y, state, use_gray);
      }
    }
  }
}

// Draw a small letter directly (for weekday and month names)
static void draw_small_letter(GContext *ctx, int letter_index, int col, int row, bool use_gray) {
  if (letter_index < 0 || letter_index > 18) return;
  const uint8_t *pattern = small_letter_patterns[letter_index];
  
  for (int r = 0; r < 5; r++) {
    for (int c = 0; c < 3; c++) {
      uint8_t state = pattern[r * 3 + c];
      if (state != CELL_EMPTY) {
        int x = s_grid_offset_x + (col + c) * CELL_SIZE;
        int y = s_grid_offset_y + (row + r) * CELL_SIZE;
        draw_cell_at(ctx, x, y, state, use_gray);
      }
    }
  }
}

// Draw separator (vertical line for date)
static void draw_separator(GContext *ctx, int col, int row, bool use_gray) {
  // 3-row vertically centered pattern: FP, PF, FP
  draw_checkerboard_2x2(ctx, col, row + 1, false, use_gray);
  draw_checkerboard_2x2(ctx, col, row + 2, true, use_gray);
  draw_cell_at(ctx, s_grid_offset_x + col * CELL_SIZE, 
               s_grid_offset_y + (row + 3) * CELL_SIZE, CELL_FULL, use_gray);
  draw_cell_at(ctx, s_grid_offset_x + (col + 1) * CELL_SIZE, 
               s_grid_offset_y + (row + 3) * CELL_SIZE, CELL_PARTIAL, use_gray);
}

// Draw colon for time
static void draw_colon(GContext *ctx, int col, int row) {
  draw_checkerboard_2x2(ctx, col, row + 1, false, true);  // Top dot
  draw_checkerboard_2x2(ctx, col, row + 4, true, true);   // Bottom dot (inverted)
}

// Draw corner decorations
static void draw_corners(GContext *ctx) {
  int x0 = s_grid_offset_x;
  int y0 = s_grid_offset_y;
  int xn = s_grid_offset_x + (s_grid_cols - 1) * CELL_SIZE;
  int yn = s_grid_offset_y + (s_grid_rows - 1) * CELL_SIZE;
  
  // Top-left: partial at corner (fg), full adjacent (secondary)
  graphics_context_set_fill_color(ctx, s_fg_color);
  graphics_fill_rect(ctx, GRect(x0 + PARTIAL_OFFSET, y0 + PARTIAL_OFFSET, PARTIAL_SIZE, PARTIAL_SIZE), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, s_secondary_color);
  graphics_fill_rect(ctx, GRect(x0 + CELL_SIZE + FULL_OFFSET, y0 + FULL_OFFSET, FULL_SIZE, FULL_SIZE), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x0 + FULL_OFFSET, y0 + CELL_SIZE + FULL_OFFSET, FULL_SIZE, FULL_SIZE), 0, GCornerNone);
  
  // Top-right
  graphics_context_set_fill_color(ctx, s_fg_color);
  graphics_fill_rect(ctx, GRect(xn + PARTIAL_OFFSET, y0 + PARTIAL_OFFSET, PARTIAL_SIZE, PARTIAL_SIZE), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, s_secondary_color);
  graphics_fill_rect(ctx, GRect(xn - CELL_SIZE + FULL_OFFSET, y0 + FULL_OFFSET, FULL_SIZE, FULL_SIZE), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(xn + FULL_OFFSET, y0 + CELL_SIZE + FULL_OFFSET, FULL_SIZE, FULL_SIZE), 0, GCornerNone);
  
  // Bottom-left
  graphics_context_set_fill_color(ctx, s_fg_color);
  graphics_fill_rect(ctx, GRect(x0 + PARTIAL_OFFSET, yn + PARTIAL_OFFSET, PARTIAL_SIZE, PARTIAL_SIZE), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, s_secondary_color);
  graphics_fill_rect(ctx, GRect(x0 + CELL_SIZE + FULL_OFFSET, yn + FULL_OFFSET, FULL_SIZE, FULL_SIZE), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x0 + FULL_OFFSET, yn - CELL_SIZE + FULL_OFFSET, FULL_SIZE, FULL_SIZE), 0, GCornerNone);
  
  // Bottom-right
  graphics_context_set_fill_color(ctx, s_fg_color);
  graphics_fill_rect(ctx, GRect(xn + PARTIAL_OFFSET, yn + PARTIAL_OFFSET, PARTIAL_SIZE, PARTIAL_SIZE), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, s_secondary_color);
  graphics_fill_rect(ctx, GRect(xn - CELL_SIZE + FULL_OFFSET, yn + FULL_OFFSET, FULL_SIZE, FULL_SIZE), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(xn + FULL_OFFSET, yn - CELL_SIZE + FULL_OFFSET, FULL_SIZE, FULL_SIZE), 0, GCornerNone);
}

// Draw step bar (5 rows x 15 cols, fills diagonally from bottom-left)
static void draw_step_bar(GContext *ctx, int col, int row) {
  const int total_cells = 75;
  int filled_cells = (s_steps * total_cells) / s_step_goal;
  if (filled_cells > total_cells) filled_cells = total_cells;
  
  int remainder = (s_steps * total_cells) % s_step_goal;
  
  int cell_index = 0;
  for (int diag = 0; diag <= 18 && cell_index < total_cells; diag++) {
    for (int c = 0; c < 15 && cell_index < total_cells; c++) {
      int r_from_bottom = diag - c;
      if (r_from_bottom >= 0 && r_from_bottom <= 4) {
        int r = 4 - r_from_bottom;
        int x = s_grid_offset_x + (col + c) * CELL_SIZE;
        int y = s_grid_offset_y + (row + r) * CELL_SIZE;
        
        bool filled = (cell_index < filled_cells) || (cell_index == filled_cells && remainder > 0);
        draw_cell_at(ctx, x, y, filled ? CELL_FULL : CELL_PARTIAL, !filled);
        cell_index++;
      }
    }
  }
}

// Draw battery indicator (2 cols x 3 rows, drains top to bottom)
static void draw_battery(GContext *ctx, int col, int row) {
  const int total_cells = 6;
  int filled_cells = (s_battery_level * total_cells) / 100;
  int remainder = (s_battery_level * total_cells) % 100;
  
  int cell_index = 0;
  for (int r = 0; r < 3; r++) {
    for (int c = 0; c < 2; c++) {
      int x = s_grid_offset_x + (col + c) * CELL_SIZE;
      int y = s_grid_offset_y + (row + r) * CELL_SIZE;
      
      // Calculate which cell from bottom (0 = bottom, 5 = top)
      int cell_from_bottom = total_cells - 1 - cell_index;
      
      if (cell_from_bottom < filled_cells) {
        // Fully filled cell - use primary color
        draw_cell_at(ctx, x, y, CELL_FULL, false);
      } else if (cell_from_bottom == filled_cells && remainder > 0) {
        // Partially filled cell (transition) - use secondary color
        draw_cell_at(ctx, x, y, CELL_PARTIAL, false);
      } else {
        // Empty (drained) cell - use secondary color
        draw_cell_at(ctx, x, y, CELL_PARTIAL, true);
      }
      cell_index++;
    }
  }
}

// Canvas update procedure - draws everything directly, no buffer
static void canvas_update_proc(Layer *layer, GContext *ctx) {
  // Draw load animation on top if active - skip normal content
  if (animations_is_active(&s_load_anim)) {
    animations_draw(ctx, &s_load_anim,
                   s_grid_cols, s_grid_rows,
                   s_grid_offset_x, s_grid_offset_y,
                   CELL_SIZE, FULL_SIZE, FULL_OFFSET,
                   PARTIAL_SIZE, PARTIAL_OFFSET,
                   s_fg_color, s_secondary_color);
    return;  // Don't draw normal content during animation
  }
  
  // Calculate layout
  int digit_spacing = (s_grid_cols > 24) ? 1 : 0;
  int time_width = (digit_spacing == 1) ? 26 : 22;
  int small_spacing = digit_spacing;
  int date_width = (small_spacing == 1) ? 16 : 14;  // Increased by 1 for 2-wide separator
  
  // Center time vertically with date below
  int time_row = (s_grid_rows - 7) / 2;  // 7 = time height
  int step_row = time_row - 5 - 2;        // 5 = step bar height, 2 = gap
  int date_row = time_row + 7 + 2;        // 2 row gap after time
  
  int time_col = (s_grid_cols - time_width) / 2;
  int date_col = (s_grid_cols - date_width) / 2 - 1;  // Moved one space left
  
  // Step bar (above time, aligned with left side of time)
  if (s_flags.show_steps && s_flags.health_available) {
    draw_step_bar(ctx, time_col, step_row);
  }
  
  // Battery indicator (right side, aligned with right edge of time, vertically centered with step bar)
  if (s_flags.show_battery) {
    int battery_col = time_col + time_width - 2;  // 2 cols wide, align right edge
    int battery_row = step_row + 1;  // Center in 5-row step area (5-3)/2 = 1
    draw_battery(ctx, battery_col, battery_row);
  }
  
  // Time digits
  int h1 = s_hour / 10;
  int h2 = s_hour % 10;
  int m1 = s_minute / 10;
  int m2 = s_minute % 10;
  
  int col = time_col;
  
  // Hour tens (use secondary color if zero)
  bool h1_use_gray = (h1 == 0);
  if (s_anim_progress[0] < 1.0f) {
    draw_digit_animated(ctx, s_anim_old_digits[0], s_anim_new_digits[0], s_anim_progress[0], col, time_row, h1_use_gray);
  } else {
    draw_digit(ctx, h1, col, time_row, h1_use_gray);
  }
  col += 5 + digit_spacing;
  
  // Hour ones
  if (s_anim_progress[1] < 1.0f) {
    draw_digit_animated(ctx, s_anim_old_digits[1], s_anim_new_digits[1], s_anim_progress[1], col, time_row, false);
  } else {
    draw_digit(ctx, h2, col, time_row, false);
  }
  col += 5 + digit_spacing;
  
  // Colon (shift right by 1 on larger screens)
  int colon_offset = 0;
  draw_colon(ctx, col + colon_offset, time_row);
  col += 2 + digit_spacing;
  
  // Minutes
  if (s_anim_progress[2] < 1.0f) {
    draw_digit_animated(ctx, s_anim_old_digits[2], s_anim_new_digits[2], s_anim_progress[2], col, time_row, false);
  } else {
    draw_digit(ctx, m1, col, time_row, false);
  }
  col += 5 + digit_spacing;
  if (s_anim_progress[3] < 1.0f) {
    draw_digit_animated(ctx, s_anim_old_digits[3], s_anim_new_digits[3], s_anim_progress[3], col, time_row, false);
  } else {
    draw_digit(ctx, m2, col, time_row, false);
  }
  
  // Date digits (if enabled)
  if (s_flags.show_date) {
    col = date_col;
    
    // Helper to draw a date component based on type
    // 0=MonthName, 1=WeekDay, 2=WeekNum, 3=Day, 4=Month, 5=Year
    for (int side = 0; side < 2; side++) {
      uint8_t date_type = (side == 0) ? s_flags.date_left : s_flags.date_right;
      
      switch (date_type) {
        case 0: // Month Name (2 letters)
          draw_small_letter(ctx, month_letters[s_month - 1][0], col, date_row, true);
          col += 3 + small_spacing;
          draw_small_letter(ctx, month_letters[s_month - 1][1], col, date_row, true);
          break;
        case 1: // Week Day (2 letters)
          draw_small_letter(ctx, weekday_letters[s_weekday][0], col, date_row, true);
          col += 3 + small_spacing;
          draw_small_letter(ctx, weekday_letters[s_weekday][1], col, date_row, true);
          break;
        case 2: // Week of the Year
          draw_small_digit(ctx, s_week / 10, col, date_row, true);
          col += 3 + small_spacing;
          draw_small_digit(ctx, s_week % 10, col, date_row, true);
          break;
        case 3: // Day
          draw_small_digit(ctx, s_day / 10, col, date_row, true);
          col += 3 + small_spacing;
          draw_small_digit(ctx, s_day % 10, col, date_row, true);
          break;
        case 4: // Month (number)
          draw_small_digit(ctx, s_month / 10, col, date_row, true);
          col += 3 + small_spacing;
          draw_small_digit(ctx, s_month % 10, col, date_row, true);
          break;
        case 5: // Year (last 2 digits)
          draw_small_digit(ctx, s_year / 10, col, date_row, true);
          col += 3 + small_spacing;
          draw_small_digit(ctx, s_year % 10, col, date_row, true);
          break;
      }
      col += 3 + small_spacing;
      
      // Draw separator after left side
      if (side == 0) {
        draw_separator(ctx, col, date_row, true);
        col += 2 + small_spacing;
      }
    }
  }
  
  // Corners
  draw_corners(ctx);
}

// Animation timer callback
static void animation_timer_callback(void *data) {
  bool still_animating = false;
  
  for (int i = 0; i < NUM_DIGITS; i++) {
    if (s_anim_progress[i] < 1.0f) {
      s_anim_progress[i] += ANIM_STEP;
      s_anim_progress[i] = (s_anim_progress[i] >= 1.0f) ? 1.0f : s_anim_progress[i];
      still_animating |= (s_anim_progress[i] < 1.0f);
    }
  }
  
  layer_mark_dirty(s_canvas_layer);
  
  if (still_animating) {
    s_anim_timer = app_timer_register(ANIM_INTERVAL_MS, animation_timer_callback, NULL);
  } else {
    s_anim_timer = NULL;
  }
}

// Update cached time
static void update_time(void) {
  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);
  
  uint8_t new_hour = (uint8_t)t->tm_hour;
  uint8_t new_minute = (uint8_t)t->tm_min;
  s_day = (uint8_t)t->tm_mday;
  s_month = (uint8_t)(t->tm_mon + 1);
  s_year = (uint8_t)(t->tm_year % 100);  // Last 2 digits of year
  
  // ISO 8601 week calculation
  // Week 1 is the week with the first Thursday of the year
  int day_of_week = t->tm_wday == 0 ? 7 : t->tm_wday; // Monday=1, Sunday=7
  int day_of_year = t->tm_yday + 1; // Make 1-indexed
  int week_number = (day_of_year - day_of_week + 10) / 7;
  
  if (week_number == 0) {
    // This day belongs to the last week of the previous year
    week_number = 52; // Simplified - could be 53 in some years
  } else if (week_number == 53) {
    // Check if this really belongs to week 1 of next year
    int jan1_day = (day_of_week - day_of_year % 7 + 7) % 7;
    if (jan1_day >= 1 && jan1_day <= 3) {
      week_number = 1;
    }
  }
  
  s_week = (uint8_t)week_number;
  
  // Store weekday (0=Monday, 6=Sunday)
  s_weekday = t->tm_wday == 0 ? 6 : t->tm_wday - 1;
  
  if (!s_flags.use_24h) {
    new_hour = new_hour % 12;
    if (new_hour == 0) new_hour = 12;
  }
  
  // Check if time digits changed and trigger animations
  if (s_prev_hour >= 0 && s_prev_minute >= 0) {
    int old_digits[NUM_DIGITS] = {s_prev_hour / 10, s_prev_hour % 10, s_prev_minute / 10, s_prev_minute % 10};
    int new_digits[NUM_DIGITS] = {new_hour / 10, new_hour % 10, new_minute / 10, new_minute % 10};
    
    bool needs_animation = false;
    for (int i = 0; i < NUM_DIGITS; i++) {
      if (old_digits[i] != new_digits[i]) {
        s_anim_old_digits[i] = old_digits[i];
        s_anim_new_digits[i] = new_digits[i];
        s_anim_progress[i] = 0.0f;
        needs_animation = true;
      }
    }
    
    if (needs_animation && !s_anim_timer) {
      s_anim_timer = app_timer_register(ANIM_INTERVAL_MS, animation_timer_callback, NULL);
    }
  }
  
  s_hour = new_hour;
  s_minute = new_minute;
  s_prev_hour = new_hour;
  s_prev_minute = new_minute;
  
  // Update step count if health is available
  if (s_flags.health_available) {
    s_steps = (uint16_t)health_service_sum_today(HealthMetricStepCount);
  }
  
  layer_mark_dirty(s_canvas_layer);
}

static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventMovementUpdate || event == HealthEventSignificantUpdate) {
    s_steps = (uint16_t)health_service_sum_today(HealthMetricStepCount);
    layer_mark_dirty(s_canvas_layer);
  }
}

static void battery_handler(BatteryChargeState charge) {
  s_battery_level = (uint8_t)charge.charge_percent;
  layer_mark_dirty(s_canvas_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

// Load settings from persistent storage
static void load_settings(void) {
  // Set defaults
  s_bg_color = GColorBlack;
  s_fg_color = GColorWhite;
  s_secondary_color = GColorLightGray;
  
  // Load from persistent storage if available
  if (persist_exists(PERSIST_KEY_BG_COLOR)) {
    s_bg_color = (GColor){ .argb = (uint8_t)persist_read_int(PERSIST_KEY_BG_COLOR) };
  }
  if (persist_exists(PERSIST_KEY_FG_COLOR)) {
    s_fg_color = (GColor){ .argb = (uint8_t)persist_read_int(PERSIST_KEY_FG_COLOR) };
  }
  if (persist_exists(PERSIST_KEY_SECONDARY_COLOR)) {
    s_secondary_color = (GColor){ .argb = (uint8_t)persist_read_int(PERSIST_KEY_SECONDARY_COLOR) };
  }
  if (persist_exists(PERSIST_KEY_STEP_GOAL)) {
    s_step_goal = (uint16_t)persist_read_int(PERSIST_KEY_STEP_GOAL);
  }
  if (persist_exists(PERSIST_KEY_SHOW_STEPS)) {
    s_flags.show_steps = persist_read_bool(PERSIST_KEY_SHOW_STEPS);
  }
  if (persist_exists(PERSIST_KEY_SHOW_BATTERY)) {
    s_flags.show_battery = persist_read_bool(PERSIST_KEY_SHOW_BATTERY);
  }
  if (persist_exists(PERSIST_KEY_SHOW_DATE)) {
    s_flags.show_date = persist_read_bool(PERSIST_KEY_SHOW_DATE);
  }
  if (persist_exists(PERSIST_KEY_USE_24H)) {
    s_flags.use_24h = persist_read_bool(PERSIST_KEY_USE_24H);
  }
  if (persist_exists(PERSIST_KEY_DATE_LEFT)) {
    s_flags.date_left = (uint8_t)persist_read_int(PERSIST_KEY_DATE_LEFT);
  }
  if (persist_exists(PERSIST_KEY_DATE_RIGHT)) {
    s_flags.date_right = (uint8_t)persist_read_int(PERSIST_KEY_DATE_RIGHT);
  }
  if (persist_exists(PERSIST_KEY_LOAD_ANIMATION)) {
    s_load_animation = (uint8_t)persist_read_int(PERSIST_KEY_LOAD_ANIMATION);
  }
}

// Save settings to persistent storage
static void save_settings(void) {
  persist_write_int(PERSIST_KEY_BG_COLOR, s_bg_color.argb);
  persist_write_int(PERSIST_KEY_FG_COLOR, s_fg_color.argb);
  persist_write_int(PERSIST_KEY_SECONDARY_COLOR, s_secondary_color.argb);
  persist_write_int(PERSIST_KEY_STEP_GOAL, s_step_goal);
  persist_write_bool(PERSIST_KEY_SHOW_STEPS, s_flags.show_steps);
  persist_write_bool(PERSIST_KEY_SHOW_BATTERY, s_flags.show_battery);
  persist_write_bool(PERSIST_KEY_SHOW_DATE, s_flags.show_date);
  persist_write_bool(PERSIST_KEY_USE_24H, s_flags.use_24h);
  persist_write_int(PERSIST_KEY_DATE_LEFT, s_flags.date_left);
  persist_write_int(PERSIST_KEY_DATE_RIGHT, s_flags.date_right);
  persist_write_int(PERSIST_KEY_LOAD_ANIMATION, s_load_animation);
}

// AppMessage inbox received handler
static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  // Background color
  Tuple *bg_t = dict_find(iter, MESSAGE_KEY_BACKGROUND_COLOR);
  if (bg_t) {
    s_bg_color = GColorFromHEX(bg_t->value->int32);
  }
  
  // Foreground color
  Tuple *fg_t = dict_find(iter, MESSAGE_KEY_FOREGROUND_COLOR);
  if (fg_t) {
    s_fg_color = GColorFromHEX(fg_t->value->int32);
  }
  
  // Secondary color
  Tuple *sec_t = dict_find(iter, MESSAGE_KEY_SECONDARY_COLOR);
  if (sec_t) {
    s_secondary_color = GColorFromHEX(sec_t->value->int32);
  }
  
  // Step goal
  Tuple *goal_t = dict_find(iter, MESSAGE_KEY_STEP_GOAL);
  if (goal_t) {
    s_step_goal = atoi(goal_t->value->cstring);
    if (s_step_goal < 1000) s_step_goal = 1000;
    if (s_step_goal > 50000) s_step_goal = 50000;
  }
  
  // Show steps
  Tuple *steps_t = dict_find(iter, MESSAGE_KEY_SHOW_STEPS);
  if (steps_t) {
    s_flags.show_steps = steps_t->value->int32 == 1;
  }
  
  // Show battery
  Tuple *batt_t = dict_find(iter, MESSAGE_KEY_SHOW_BATTERY);
  if (batt_t) {
    s_flags.show_battery = batt_t->value->int32 == 1;
  }
  
  // Show date
  Tuple *date_t = dict_find(iter, MESSAGE_KEY_SHOW_DATE);
  if (date_t) {
    s_flags.show_date = date_t->value->int32 == 1;
  }
  
  // Use 24-hour format
  Tuple *hour_t = dict_find(iter, MESSAGE_KEY_USE_24_HOUR);
  if (hour_t) {
    s_flags.use_24h = hour_t->value->int32 == 1;
  }
  
  // Date left side
  Tuple *date_left_t = dict_find(iter, MESSAGE_KEY_DATE_LEFT);
  if (date_left_t) {
    int val = atoi(date_left_t->value->cstring);
    if (val >= 0 && val <= 5) s_flags.date_left = (uint8_t)val;
  }
  
  // Date right side
  Tuple *date_right_t = dict_find(iter, MESSAGE_KEY_DATE_RIGHT);
  if (date_right_t) {
    int val = atoi(date_right_t->value->cstring);
    if (val >= 0 && val <= 5) s_flags.date_right = (uint8_t)val;
  }
  
  // Load animation
  Tuple *anim_t = dict_find(iter, MESSAGE_KEY_LOAD_ANIMATION);
  if (anim_t) {
    int anim_val = atoi(anim_t->value->cstring);
    if (anim_val < 0) anim_val = 0;
    if (anim_val > 3) anim_val = 3;
    s_load_animation = (uint8_t)anim_val;
  }
  
  // Save and update
  save_settings();
  window_set_background_color(s_window, s_bg_color);
  layer_mark_dirty(s_canvas_layer);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  s_grid_cols = bounds.size.w / CELL_SIZE;
  s_grid_rows = bounds.size.h / CELL_SIZE;
  s_grid_offset_x = (bounds.size.w - s_grid_cols * CELL_SIZE) / 2;
  s_grid_offset_y = (bounds.size.h - s_grid_rows * CELL_SIZE) / 2;
  
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);
  
  // Initialize and start load animation based on setting
  animations_init(&s_load_anim);
  s_load_anim.layer = s_canvas_layer;
  
  if (s_load_animation == 1) {
    animations_start_load(&s_load_anim, ANIM_WAVE_FILL);
  } else if (s_load_animation == 2) {
    animations_start_load(&s_load_anim, ANIM_RANDOM_POP);
  } else if (s_load_animation == 3) {
    animations_start_load(&s_load_anim, ANIM_MATRIX);
  }
  // If s_load_animation == 0, don't start any animation
  
  update_time();
}

static void prv_window_unload(Window *window) {
  animations_stop(&s_load_anim);
  layer_destroy(s_canvas_layer);
}

static void prv_init(void) {
  // Load saved settings
  load_settings();
  
  s_window = window_create();
  window_set_background_color(s_window, s_bg_color);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  window_stack_push(s_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Subscribe to battery service
  battery_state_service_subscribe(battery_handler);
  s_battery_level = (uint8_t)battery_state_service_peek().charge_percent;
  
  // Subscribe to health only if available
  s_flags.health_available = health_service_events_subscribe(health_handler, NULL);
  
  // Open AppMessage for settings
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(128, 64);
}

static void prv_deinit(void) {
  if (s_anim_timer) {
    app_timer_cancel(s_anim_timer);
  }
  if (s_flags.health_available) {
    health_service_events_unsubscribe();
  }
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
