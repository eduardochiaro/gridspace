#include <pebble.h>

// Grid cell size
#define CELL_SIZE 6

// Cell states
typedef enum {
  CELL_EMPTY = 0,
  CELL_PARTIAL = 1,
  CELL_FULL = 2,
  CELL_GRAY = 3,
  CELL_GRAY_PARTIAL = 4
} CellState;

static Window *s_window;
static Layer *s_canvas_layer;

// Grid dimensions (calculated based on screen size)
static int s_grid_cols;
static int s_grid_rows;
static int s_grid_offset_x;
static int s_grid_offset_y;

// Grid data
static CellState *s_grid_data;

// Draw a single cell
static void draw_cell(GContext *ctx, int col, int row, CellState state) {
  int x = s_grid_offset_x + col * CELL_SIZE;
  int y = s_grid_offset_y + row * CELL_SIZE;
  
  switch(state) {
    case CELL_FULL:
      // Draw 4x4 square at center (1 pixel padding on each side)
      graphics_context_set_fill_color(ctx, GColorWhite);
      graphics_fill_rect(ctx, GRect(x + 1, y + 1, 4, 4), 0, GCornerNone);
      break;
      
    case CELL_PARTIAL:
      // Draw 2x2 square at center (2 pixel padding on each side)
      graphics_context_set_fill_color(ctx, GColorWhite);
      graphics_fill_rect(ctx, GRect(x + 2, y + 2, 2, 2), 0, GCornerNone);
      break;
      
    case CELL_GRAY:
      // Draw 4x4 square in gray
      graphics_context_set_fill_color(ctx, GColorLightGray);
      graphics_fill_rect(ctx, GRect(x + 1, y + 1, 4, 4), 0, GCornerNone);
      break;
      
    case CELL_GRAY_PARTIAL:
      // Draw 2x2 square in gray
      graphics_context_set_fill_color(ctx, GColorLightGray);
      graphics_fill_rect(ctx, GRect(x + 2, y + 2, 2, 2), 0, GCornerNone);
      break;
      
    case CELL_EMPTY:
    default:
      // Nothing to draw
      break;
  }
}

// Canvas update procedure
static void canvas_update_proc(Layer *layer, GContext *ctx) {
  // Draw all cells
  for (int row = 0; row < s_grid_rows; row++) {
    for (int col = 0; col < s_grid_cols; col++) {
      CellState state = s_grid_data[row * s_grid_cols + col];
      draw_cell(ctx, col, row, state);
    }
  }
}

// Small digit patterns (3x5 for each digit 0-9) - half size for date display
// 0 = CELL_EMPTY, 1 = CELL_PARTIAL (edges/curves), 2 = CELL_FULL
static const uint8_t small_digit_patterns[10][15] = {
  // 0
  {1,2,1,
   2,0,2,
   2,0,2,
   2,0,2,
   1,2,1},
  // 1
  {1,2,0,
   0,2,0,
   0,2,0,
   0,2,0,
   1,2,1},
  // 2
  {1,2,1,
   0,0,2,
   1,2,1,
   2,0,0,
   1,2,1},
  // 3
  {1,2,1,
   0,0,2,
   0,2,1,
   0,0,2,
   1,2,1},
  // 4
  {1,0,1,
   2,0,2,
   1,2,2,
   0,0,2,
   0,0,1},
  // 5
  {1,2,1,
   2,0,0,
   1,2,1,
   0,0,2,
   1,2,1},
  // 6
  {1,2,1,
   2,0,0,
   2,2,1,
   2,0,2,
   1,2,1},
  // 7
  {1,2,1,
   0,0,2,
   0,0,1,
   0,2,0,
   0,1,0},
  // 8
  {1,2,1,
   2,0,2,
   1,2,1,
   2,0,2,
   1,2,1},
  // 9
  {1,2,1,
   2,0,2,
   1,2,2,
   0,0,2,
   1,2,1}
};

// Digit patterns (5x7 for each digit 0-9)
// 0 = CELL_EMPTY, 1 = CELL_PARTIAL (edges/curves), 2 = CELL_FULL
static const uint8_t digit_patterns[10][35] = {
  // 0
  {1,2,2,2,1,
   2,0,0,0,2,
   2,0,0,0,2,
   2,0,0,0,2,
   2,0,0,0,2,
   2,0,0,0,2,
   1,2,2,2,1},
  // 1
  {0,0,2,0,0,
   1,2,2,0,0,
   0,0,2,0,0,
   0,0,2,0,0,
   0,0,2,0,0,
   0,0,2,0,0,
   1,2,2,2,1},
  // 2
  {1,2,2,2,1,
   0,0,0,0,2,
   0,0,0,0,2,
   1,2,2,2,1,
   2,0,0,0,0,
   2,0,0,0,0,
   1,2,2,2,1},
  // 3
  {1,2,2,2,1,
   0,0,0,0,2,
   0,0,0,0,2,
   0,1,2,2,1,
   0,0,0,0,2,
   0,0,0,0,2,
   1,2,2,2,1},
  // 4
  {1,0,0,0,1,
   2,0,0,0,2,
   2,0,0,0,2,
   1,2,2,2,2,
   0,0,0,0,2,
   0,0,0,0,2,
   0,0,0,0,1},
  // 5
  {1,2,2,2,1,
   2,0,0,0,0,
   2,0,0,0,0,
   1,2,2,2,1,
   0,0,0,0,2,
   0,0,0,0,2,
   1,2,2,2,1},
  // 6
  {1,2,2,2,1,
   2,0,0,0,0,
   2,0,0,0,0,
   2,2,2,2,1,
   2,0,0,0,2,
   2,0,0,0,2,
   1,2,2,2,1},
  // 7
  {1,2,2,2,1,
   0,0,0,0,2,
   0,0,0,0,2,
   0,0,0,0,1,
   0,0,0,2,0,
   0,0,0,2,0,
   0,0,0,1,0},
  // 8
  {1,2,2,2,1,
   2,0,0,0,2,
   2,0,0,0,2,
   1,2,2,2,1,
   2,0,0,0,2,
   2,0,0,0,2,
   1,2,2,2,1},
  // 9
  {1,2,2,2,1,
   2,0,0,0,2,
   2,0,0,0,2,
   1,2,2,2,2,
   0,0,0,0,2,
   0,0,0,0,2,
   1,2,2,2,1}
};

// Draw a digit at the specified grid position with optional gray color
static void draw_digit_to_grid_colored(int digit, int start_col, int start_row, bool use_gray) {
  if (digit < 0 || digit > 9) return;
  
  // Draw the digit using the pattern values directly
  for (int row = 0; row < 7; row++) {
    for (int col = 0; col < 5; col++) {
      int grid_col = start_col + col;
      int grid_row = start_row + row;
      
      if (grid_row >= 0 && grid_row < s_grid_rows && grid_col >= 0 && grid_col < s_grid_cols) {
        uint8_t pattern_value = digit_patterns[digit][row * 5 + col];
        CellState state = pattern_value;
        
        // Convert to gray if needed
        if (use_gray && pattern_value > 0) {
          state = (pattern_value == 2) ? CELL_GRAY : CELL_GRAY_PARTIAL;
        }
        
        s_grid_data[grid_row * s_grid_cols + grid_col] = state;
      }
    }
  }
}

// Draw a digit at the specified grid position
static void draw_digit_to_grid(int digit, int start_col, int start_row) {
  draw_digit_to_grid_colored(digit, start_col, start_row, false);
}

// Draw a small digit at the specified grid position with optional gray color
static void draw_small_digit_to_grid_colored(int digit, int start_col, int start_row, bool use_gray) {
  if (digit < 0 || digit > 9) return;
  
  // Draw the digit using the small pattern values (3x5)
  for (int row = 0; row < 5; row++) {
    for (int col = 0; col < 3; col++) {
      int grid_col = start_col + col;
      int grid_row = start_row + row;
      
      if (grid_row >= 0 && grid_row < s_grid_rows && grid_col >= 0 && grid_col < s_grid_cols) {
        uint8_t pattern_value = small_digit_patterns[digit][row * 3 + col];
        CellState state = pattern_value;
        
        // Convert to gray if needed
        if (use_gray && pattern_value > 0) {
          state = (pattern_value == 2) ? CELL_GRAY : CELL_GRAY_PARTIAL;
        }
        
        s_grid_data[grid_row * s_grid_cols + grid_col] = state;
      }
    }
  }
}

// Draw a small digit at the specified grid position
static void draw_small_digit_to_grid(int digit, int start_col, int start_row) {
  draw_small_digit_to_grid_colored(digit, start_col, start_row, false);
}

// Draw small slash separator for date (3 cols x 5 rows)
static void draw_small_slash_to_grid(int start_col, int start_row, bool use_gray) {
  // Slash pattern: diagonal from bottom-left to top-right
  // Pattern (3x5):
  // 0 0 1
  // 0 0 2
  // 0 2 0
  // 2 0 0
  // 1 0 0
  static const uint8_t slash_pattern[15] = {
    0,0,0,
    0,1,0,
    0,2,0,
    0,1,0,
    0,0,0
  };
  
  for (int row = 0; row < 5; row++) {
    for (int col = 0; col < 3; col++) {
      int grid_col = start_col + col;
      int grid_row = start_row + row;
      
      if (grid_row >= 0 && grid_row < s_grid_rows && grid_col >= 0 && grid_col < s_grid_cols) {
        uint8_t pattern_value = slash_pattern[row * 3 + col];
        if (pattern_value > 0) {
          CellState state = pattern_value;
          if (use_gray) {
            state = (pattern_value == 2) ? CELL_GRAY : CELL_GRAY_PARTIAL;
          }
          s_grid_data[grid_row * s_grid_cols + grid_col] = state;
        }
      }
    }
  }
}

// Draw colon separator
static void draw_colon_to_grid(int start_col, int start_row) {
  // Draw two dots, each composed of 4 gray partial cells (2x2)
  // Position dots vertically centered, shifted up by 1
  int dot_rows[] = {1, 4}; // Moved up by 1 grid space
  
  for (int i = 0; i < 2; i++) {
    int base_row = start_row + dot_rows[i];
    int base_col = start_col;
    
    // Draw 2x2 block of gray partial cells
    for (int row = 0; row < 2; row++) {
      for (int col = 0; col < 2; col++) {
        int grid_row = base_row + row;
        int grid_col = base_col + col;
        
        if (grid_row >= 0 && grid_row < s_grid_rows && 
            grid_col >= 0 && grid_col < s_grid_cols) {
          s_grid_data[grid_row * s_grid_cols + grid_col] = CELL_GRAY_PARTIAL;
        }
      }
    }
  }
}

// Initialize grid with empty values
static void init_grid_empty(void) {
  for (int i = 0; i < s_grid_rows * s_grid_cols; i++) {
    s_grid_data[i] = CELL_EMPTY;
  }
}

// Draw corner decorations
static void draw_corners(void) {
  // Top-left corner: < shape pointing inward
  if (s_grid_rows > 1 && s_grid_cols > 1) {
    s_grid_data[0 * s_grid_cols + 0] = CELL_PARTIAL;
    s_grid_data[0 * s_grid_cols + 1] = CELL_FULL;
    s_grid_data[1 * s_grid_cols + 0] = CELL_FULL;
  }
  
  // Top-right corner: > shape pointing inward
  if (s_grid_rows > 1 && s_grid_cols > 1) {
    s_grid_data[0 * s_grid_cols + (s_grid_cols - 2)] = CELL_FULL;
    s_grid_data[0 * s_grid_cols + (s_grid_cols - 1)] = CELL_PARTIAL;
    s_grid_data[1 * s_grid_cols + (s_grid_cols - 1)] = CELL_FULL;
  }
  
  // Bottom-left corner: < shape rotated pointing inward
  if (s_grid_rows > 1 && s_grid_cols > 1) {
    s_grid_data[(s_grid_rows - 2) * s_grid_cols + 0] = CELL_FULL;
    s_grid_data[(s_grid_rows - 1) * s_grid_cols + 0] = CELL_PARTIAL;
    s_grid_data[(s_grid_rows - 1) * s_grid_cols + 1] = CELL_FULL;
  }
  
  // Bottom-right corner: > shape rotated pointing inward
  if (s_grid_rows > 1 && s_grid_cols > 1) {
    s_grid_data[(s_grid_rows - 2) * s_grid_cols + (s_grid_cols - 1)] = CELL_FULL;
    s_grid_data[(s_grid_rows - 1) * s_grid_cols + (s_grid_cols - 2)] = CELL_FULL;
    s_grid_data[(s_grid_rows - 1) * s_grid_cols + (s_grid_cols - 1)] = CELL_PARTIAL;
  }
}

// Update time display on grid
static void update_time(void) {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  int hour = tick_time->tm_hour;
  int minute = tick_time->tm_min;
  int day = tick_time->tm_mday;
  int month = tick_time->tm_mon + 1; // tm_mon is 0-based
  
  // Convert to 12-hour format if needed
  if (!clock_is_24h_style()) {
    hour = hour % 12;
    if (hour == 0) hour = 12;
  }
  
  // Clear grid
  init_grid_empty();
  
  // Calculate digits for time
  int h1 = hour / 10;
  int h2 = hour % 10;
  int m1 = minute / 10;
  int m2 = minute % 10;
  
  // Calculate digits for date
  int d1 = day / 10;
  int d2 = day % 10;
  int mo1 = month / 10;
  int mo2 = month % 10;
  
  // Adjust spacing based on screen size (smaller screens = no spacing)
  // Screens 144 wide or less (Aplite, Basalt, Diorite) use no spacing
  // Larger screens (Chalk 180, Emery 200) use spacing
  int digit_spacing = (s_grid_cols > 24) ? 1 : 0;
  
  // Calculate colon spacing - no space after colon on small screens
  int colon_spacing = (digit_spacing == 1) ? digit_spacing : 0;
  
  // Calculate time width based on spacing
  // With spacing: 5 + 1 + 5 + 1 + 2 + 1 + 5 + 1 + 5 = 26 cols
  // Without spacing: 5 + 5 + 2 + 5 + 5 = 22 cols
  int time_width = (digit_spacing == 1) ? 26 : 22;
  
  // Calculate date width (small digits: 3 wide each, slash: 3 wide)
  // Format: DD/MM - 3 + 1 + 3 + 1 + 3 + 1 + 3 + 1 + 3 = 19 cols with spacing
  // Without spacing: 3 + 3 + 3 + 3 + 3 = 15 cols
  int small_digit_spacing = (s_grid_cols > 24) ? 1 : 0;
  int date_width = (small_digit_spacing == 1) ? 19 : 15;
  
  // Calculate vertical positioning
  // Time: 7 rows, Date: 5 rows, Gap: 2 rows = 14 total
  int total_height = 7 + 2 + 5; // time + gap + date
  int start_row = (s_grid_rows - total_height) / 2;
  int date_row = start_row + 7 + 2; // 2 rows gap after time
  
  int start_col = (s_grid_cols - time_width) / 2;
  int date_start_col = (s_grid_cols - date_width) / 2;
  
  int current_col = start_col;
  
  // Check if display supports color (hide gray elements on monochrome displays)
  bool supports_color = PBL_IF_COLOR_ELSE(true, false);
  
  // Draw hours (show leading zero in gray on color displays, hide on B&W)
  if (h1 == 0) {
    if (supports_color) {
      draw_digit_to_grid_colored(0, current_col, start_row, true);
    }
    // else: skip drawing on B&W displays
  } else {
    draw_digit_to_grid(h1, current_col, start_row);
  }
  current_col += 5 + digit_spacing; // digit width + optional space
  
  draw_digit_to_grid(h2, current_col, start_row);
  current_col += 5 + digit_spacing;
  
  // Draw colon (only on color displays)
  if (supports_color) {
    draw_colon_to_grid(current_col, start_row);
  }
  current_col += 2 + colon_spacing;
  
  // Draw minutes (always show both digits)
  draw_digit_to_grid(m1, current_col, start_row);
  current_col += 5 + digit_spacing;
  draw_digit_to_grid(m2, current_col, start_row);
  
  // Draw date below time (gray on color displays, white on B&W)
  {
    int date_col = date_start_col;
    
    // Draw day
    draw_small_digit_to_grid_colored(d1, date_col, date_row, supports_color);
    date_col += 3 + small_digit_spacing;
    draw_small_digit_to_grid_colored(d2, date_col, date_row, supports_color);
    date_col += 3 + small_digit_spacing;
    
    // Draw slash separator
    draw_small_slash_to_grid(date_col, date_row, supports_color);
    date_col += 3 + small_digit_spacing;
    
    // Draw month
    draw_small_digit_to_grid_colored(mo1, date_col, date_row, supports_color);
    date_col += 3 + small_digit_spacing;
    draw_small_digit_to_grid_colored(mo2, date_col, date_row, supports_color);
  }
  
  // Draw corner decorations
  draw_corners();
  
  // Request layer redraw
  layer_mark_dirty(s_canvas_layer);
}

// Tick handler
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Calculate grid dimensions to fill the screen
  s_grid_cols = bounds.size.w / CELL_SIZE;
  s_grid_rows = bounds.size.h / CELL_SIZE;
  
  // Calculate the actual grid size in pixels
  int grid_width = s_grid_cols * CELL_SIZE;
  int grid_height = s_grid_rows * CELL_SIZE;
  
  // Center the grid by calculating remaining space
  s_grid_offset_x = (bounds.size.w - grid_width) / 2;
  s_grid_offset_y = (bounds.size.h - grid_height) / 2;
  
  // Allocate grid data
  s_grid_data = malloc(s_grid_rows * s_grid_cols * sizeof(CellState));
  
  // Initialize grid with empty values
  init_grid_empty();
  
  // Create canvas layer
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);
  
  // Initial time update
  update_time();
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Grid watchface initialized: %dx%d cells", 
          s_grid_cols, s_grid_rows);
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  free(s_grid_data);
}

static void prv_init(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void prv_deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Grid watchface initialized");

  app_event_loop();
  prv_deinit();
}
